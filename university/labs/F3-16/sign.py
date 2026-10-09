#!/usr/bin/env python3
"""sign.py - Authenticode-sign a PE32+ UEFI image, written out by hand so every step is visible.

    sign.py <in.efi> <out.efi> <cert.pem> <key.pem> <passphrase>
    sign.py --hash <file.efi>        print the Authenticode SHA-256 of an image (signed or not)

Steps: (1) the Authenticode hash of the image: the headers without the checksum field and the
certificate-table directory entry, then every section's raw data in file order; (2) a DER
SpcIndirectDataContent holding that hash; (3) a PKCS#7 SignedData whose signed attributes hold
the SHA-256 of (2), signed with RSA PKCS#1 v1.5; (4) a WIN_CERTIFICATE appended at the end of the
file, 8-byte aligned, and the certificate-table directory entry pointing at it.
Formats are from memory of the Microsoft PE/COFF and Authenticode documents and PKCS#7
(pending verification); the run in run.sh is the test: the firmware accepts or refuses the result.
Only the RSA signature itself uses a library (python3-cryptography).
"""
import hashlib
import struct
import sys

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding


def der(tag, body):
    n = len(body)
    if n < 0x80:
        length = bytes([n])
    else:
        b = n.to_bytes((n.bit_length() + 7) // 8, "big")
        length = bytes([0x80 | len(b)]) + b
    return bytes([tag]) + length + body


def seq(*parts):
    return der(0x30, b"".join(parts))


def oid(dotted):
    nums = [int(x) for x in dotted.split(".")]
    body = bytes([40 * nums[0] + nums[1]])
    for n in nums[2:]:
        chunk = [n & 0x7F]
        n >>= 7
        while n:
            chunk.append(0x80 | (n & 0x7F))
            n >>= 7
        body += bytes(reversed(chunk))
    return der(0x06, body)


SHA256 = seq(oid("2.16.840.1.101.3.4.2.1"), b"\x05\x00")
SPC_INDIRECT_DATA = "1.3.6.1.4.1.311.2.1.4"
SPC_PE_IMAGE_DATA = "1.3.6.1.4.1.311.2.1.15"


def pe_layout(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe:pe + 4] == b"PE\0\0", "not a PE file"
    coff = pe + 4
    nsections, opt_size = struct.unpack_from("<H", data, coff + 2)[0], struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    assert struct.unpack_from("<H", data, opt)[0] == 0x20B, "not PE32+"
    checksum = opt + 64
    size_of_headers = struct.unpack_from("<I", data, opt + 60)[0]
    certdir = opt + 112 + 4 * 8          # data directory 4: the certificate table
    sections = []
    for i in range(nsections):
        s = opt + opt_size + 40 * i
        raw_size, raw_ptr = struct.unpack_from("<II", data, s + 16)
        if raw_size:
            sections.append((raw_ptr, raw_size))
    return checksum, certdir, size_of_headers, sorted(sections)


def authenticode_hash(data):
    checksum, certdir, size_of_headers, sections = pe_layout(data)
    cert_off, cert_size = struct.unpack_from("<II", data, certdir)
    h = hashlib.sha256()
    h.update(data[:checksum])
    h.update(data[checksum + 4:certdir])
    h.update(data[certdir + 8:size_of_headers])
    end = size_of_headers
    for ptr, size in sections:
        h.update(data[ptr:ptr + size])
        end = max(end, ptr + size)
    tail_end = cert_off if cert_size else len(data)
    if tail_end > end:                    # bytes after the last section, before the certificates
        h.update(data[end:tail_end])
    return h.digest()


def sign(data, cert, key):
    image_hash = authenticode_hash(data)
    spc_pe_image_data = seq(der(0x03, b"\x00"),                       # flags: empty bit string
                            der(0xA0, der(0xA2, der(0x80, b""))))     # file: <<<Obsolete>>> left empty
    content = seq(seq(oid(SPC_PE_IMAGE_DATA), spc_pe_image_data),
                  seq(SHA256, der(0x04, image_hash)))
    content_value = content[2:] if content[1] < 0x80 else content[2 + (content[1] & 0x7F):]
    attrs = [seq(oid("1.2.840.113549.1.9.3"), der(0x31, oid(SPC_INDIRECT_DATA))),                    # contentType
             seq(oid("1.2.840.113549.1.9.4"), der(0x31, der(0x04, hashlib.sha256(content_value).digest())))]  # messageDigest
    attrs.sort()                                                       # DER: SET OF in sorted order
    signed_attrs = der(0x31, b"".join(attrs))
    signature = key.sign(signed_attrs, padding.PKCS1v15(), hashes.SHA256())
    cert_der = cert.public_bytes(serialization.Encoding.DER)
    serial = cert.serial_number
    serial_der = der(0x02, serial.to_bytes((serial.bit_length() + 8) // 8, "big"))
    signer = seq(der(0x02, b"\x01"),
                 seq(cert.issuer.public_bytes(), serial_der),
                 SHA256,
                 der(0xA0, b"".join(attrs)),                           # [0] IMPLICIT: same bytes, other tag
                 seq(oid("1.2.840.113549.1.1.1"), b"\x05\x00"),
                 der(0x04, signature))
    signed_data = seq(der(0x02, b"\x01"),
                      der(0x31, SHA256),
                      seq(oid(SPC_INDIRECT_DATA), der(0xA0, content)),
                      der(0xA0, cert_der),
                      der(0x31, signer))
    pkcs7 = seq(oid("1.2.840.113549.1.7.2"), der(0xA0, signed_data))
    return image_hash, pkcs7


def main():
    if sys.argv[1] == "--hash":
        print(authenticode_hash(open(sys.argv[2], "rb").read()).hex())
        return
    src, dst, cert_path, key_path, passphrase = sys.argv[1:6]
    data = bytearray(open(src, "rb").read())
    _, certdir, _, _ = pe_layout(data)
    assert struct.unpack_from("<II", data, certdir) == (0, 0), "already signed"
    data += b"\0" * (-len(data) % 8)                                   # certificate table is 8-byte aligned
    cert = x509.load_pem_x509_certificate(open(cert_path, "rb").read())
    key = serialization.load_pem_private_key(open(key_path, "rb").read(), passphrase.encode())
    image_hash, pkcs7 = sign(bytes(data), cert, key)
    win_cert = struct.pack("<IHH", 8 + len(pkcs7), 0x0200, 0x0002) + pkcs7   # WIN_CERT_TYPE_PKCS_SIGNED_DATA
    win_cert += b"\0" * (-len(win_cert) % 8)
    struct.pack_into("<II", data, certdir, len(data), len(win_cert))
    data += win_cert
    open(dst, "wb").write(data)
    print("Authenticode SHA-256 of the image: %s" % image_hash.hex())
    print("signer: %s" % cert.subject.rfc4514_string())
    print("certificate table: offset 0x%x, %d bytes (PKCS#7 SignedData %d bytes)"
          % (struct.unpack_from("<I", data, certdir)[0], len(win_cert), len(pkcs7)))


if __name__ == "__main__":
    main()
