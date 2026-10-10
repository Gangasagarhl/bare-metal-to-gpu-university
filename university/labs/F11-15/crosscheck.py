# F11-15 Listing 4: the same four values computed by Python's hashlib and hmac modules.
import hashlib
import hmac
print("sha256 abc        " + hashlib.sha256(b"abc").hexdigest())
print("sha256 a*1000     " + hashlib.sha256(b"a" * 1000).hexdigest())
LAB_KEY = b"robot-key-for-the-lab"
LAB_MSG = b"seq=7;cmd=drive;v=0.30;w=0.00"
print("hmac lab-command  " + hmac.new(LAB_KEY, LAB_MSG, hashlib.sha256).hexdigest())
print("hmac 100-byte-key " + hmac.new(b"k" * 100, b"long key", hashlib.sha256).hexdigest())
