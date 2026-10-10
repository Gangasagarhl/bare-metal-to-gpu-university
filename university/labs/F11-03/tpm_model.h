// tpm_model.h - F11-03 Listing 1: the university's model of a TPM's PCR bank, an event log,
// a signed quote and PCR-bound sealing. It models the IDEAS of measured boot and attestation;
// it does not implement the TCG data formats or the TPM command set. Hashing uses sha256.h;
// the attestation key is ECDSA P-256 through OpenSSL's EVP interface.
#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/ec.h>
#include <openssl/evp.h>

#include "sha256.h"

using Digest = std::array<uint8_t, 32>;
using Bytes = std::vector<uint8_t>;

inline Digest sha256(const Bytes& data)
{
    Sha256 h;
    h.update(data.data(), data.size());
    Digest d{};
    h.finish(d.data());
    return d;
}

inline Digest sha256(const std::string& text)
{
    return sha256(Bytes(text.begin(), text.end()));
}

inline std::string hex(const Digest& d, size_t n = 4)
{
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        s += "0123456789abcdef"[d[i] >> 4];
        s += "0123456789abcdef"[d[i] & 0xf];
    }
    return s;
}

constexpr int kPcrCount = 10;  // this model keeps PCRs 0-9 only (a real TPM has more)

// One measurement: which PCR, what kind of thing, its digest, a human-readable description.
struct Event {
    int pcr;
    std::string type;
    Digest digest;
    std::string description;
};

// The PCR bank: registers start at zero and can only be extended.
class PcrBank {
public:
    void extend(int pcr, const Digest& d)
    {
        Bytes buf(pcr_[pcr].begin(), pcr_[pcr].end());
        buf.insert(buf.end(), d.begin(), d.end());
        pcr_[pcr] = sha256(buf);           // PCR_new = SHA-256(PCR_old || digest)
    }
    const Digest& read(int pcr) const { return pcr_[pcr]; }

private:
    std::array<Digest, kPcrCount> pcr_{};
};

// Replaying a log: what the PCRs must hold if the log is complete and honest.
inline PcrBank replay(const std::vector<Event>& log)
{
    PcrBank bank;
    for (const Event& e : log) {
        bank.extend(e.pcr, e.digest);
    }
    return bank;
}

// Composite digest of a PCR selection: SHA-256 over the selected values in order.
inline Digest composite(const PcrBank& bank, const std::vector<int>& selection)
{
    Bytes buf;
    for (int p : selection) {
        buf.insert(buf.end(), bank.read(p).begin(), bank.read(p).end());
    }
    return sha256(buf);
}

struct PkeyFree {
    void operator()(EVP_PKEY* k) const { EVP_PKEY_free(k); }
};
struct MdCtxFree {
    void operator()(EVP_MD_CTX* c) const { EVP_MD_CTX_free(c); }
};
using Pkey = std::unique_ptr<EVP_PKEY, PkeyFree>;
using MdCtx = std::unique_ptr<EVP_MD_CTX, MdCtxFree>;

// A quote: the TPM signs (nonce || composite digest of the selected PCRs) with its attestation key.
struct Quote {
    Bytes nonce;
    std::vector<int> selection;
    Digest pcr_digest;
    Bytes signature;
};

class Tpm {
public:
    Tpm() : key_(EVP_EC_gen("P-256"))
    {
        if (!key_) {
            throw std::runtime_error("EVP_EC_gen failed");
        }
    }
    PcrBank& pcrs() { return bank_; }
    EVP_PKEY* attestation_key() const { return key_.get(); }   // the verifier gets the public half

    Quote quote(const Bytes& nonce, const std::vector<int>& selection) const
    {
        Quote q{nonce, selection, composite(bank_, selection), {}};
        Bytes msg = nonce;
        msg.insert(msg.end(), q.pcr_digest.begin(), q.pcr_digest.end());
        MdCtx ctx(EVP_MD_CTX_new());
        size_t len = 0;
        if (!ctx || EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, key_.get()) != 1 ||
            EVP_DigestSign(ctx.get(), nullptr, &len, msg.data(), msg.size()) != 1) {
            throw std::runtime_error("signing failed");
        }
        q.signature.resize(len);
        if (EVP_DigestSign(ctx.get(), q.signature.data(), &len, msg.data(), msg.size()) != 1) {
            throw std::runtime_error("signing failed");
        }
        q.signature.resize(len);
        return q;
    }

    // Sealing: a secret is released only while the selected PCRs hold the values sealed to.
    struct Sealed {
        std::vector<int> selection;
        Digest policy;
        std::string secret;
    };
    Sealed seal(const std::string& secret, const std::vector<int>& selection) const
    {
        return {selection, composite(bank_, selection), secret};
    }
    bool unseal(const Sealed& s, std::string& out) const
    {
        if (composite(bank_, s.selection) != s.policy) {
            return false;
        }
        out = s.secret;
        return true;
    }

private:
    PcrBank bank_;
    Pkey key_;
};

inline bool verify_signature(EVP_PKEY* key, const Quote& q)
{
    Bytes msg = q.nonce;
    msg.insert(msg.end(), q.pcr_digest.begin(), q.pcr_digest.end());
    MdCtx ctx(EVP_MD_CTX_new());
    return ctx && EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, key) == 1 &&
           EVP_DigestVerify(ctx.get(), q.signature.data(), q.signature.size(), msg.data(), msg.size()) == 1;
}
