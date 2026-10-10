// shared.h - F11-12: the one declaration the driver and the tests share.
#ifndef F11_12_SHARED_H
#define F11_12_SHARED_H
// Parse a device configuration descriptor blob. Returns the number of
// descriptors found, or -1 if the input is rejected as malformed.
int parse_config(const unsigned char* data, unsigned long size);
#endif
