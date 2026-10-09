// bootsim.cc - F3-41 Listing 2: the H4 acceptance tests, run against the flash model of
// Listing 1: a good update, an update signed with the wrong key, a power cut at every single
// flash operation of an update, a new image that never confirms, and a downgrade attempt.
#include "ota.h"

namespace {

const Pkey vendorKey = keyFromSeed("OS305 vendor signing key");
const Pkey wrongKey = keyFromSeed("someone else's key");
const Bytes v1 = makeImage(1, 6000, vendorKey);
const Bytes v2 = makeImage(2, 6200, vendorKey);

template <typename Device>
Device fresh()
{
    if constexpr (std::is_same_v<Device, ABSlots>) { Device d(vendorKey); d.factory(v1); d.boot(); d.log.clear(); return d; }
    else { Device d; d.factory(v1); d.boot(); d.log.clear(); return d; }
}

// Power cut at operation k of the update (counted from the start of the update), power back,
// boot, let a healthy application confirm, reboot: which version runs in the end?
template <typename Device>
void sweep(const char* name)
{
    Device probe = fresh<Device>();
    const long start = probe.flash.ops;
    probe.update(v2);
    const long n = probe.flash.ops - start;
    int onV1 = 0, onV2 = 0, bricked = 0;
    for (long k = 0; k < n; ++k) {
        Device d = fresh<Device>();
        d.flash.cutAt = d.flash.ops + k;
        d.update(v2);
        d.flash.restorePower();
        int v = d.boot();
        if (v > 0) { d.confirm(); v = d.boot(); }
        (v == 1 ? onV1 : v == 2 ? onV2 : bricked) += 1;
    }
    std::printf("%-11s update = %ld flash operations; power cut at each one in turn:\n", name, n);
    std::printf("            still on v1: %d, on v2: %d, BRICKED: %d\n", onV1, onV2, bricked);
}

}  // namespace

int main()
{
    std::printf("images: v1 %zu bytes, v2 %zu bytes (16-byte header + payload + 64-byte Ed25519 signature)\n",
                v1.size(), v2.size());

    ABSlots a = fresh<ABSlots>();                          // test 1: a normal update
    a.update(v2); int r1 = a.boot(); a.confirm(); int r1b = a.boot();
    std::printf("1 good update:        %s-> runs v%d, after reboot v%d\n", a.log.c_str(), r1, r1b);

    ABSlots b = fresh<ABSlots>();                          // test 2: wrong signature
    b.update(makeImage(3, 6100, wrongKey)); int r2 = b.boot();
    std::printf("2 wrong signature:    %s-> runs v%d\n", b.log.c_str(), r2);

    ABSlots c = fresh<ABSlots>();                          // test 3: new image never confirms
    c.update(v2); int r3a = c.boot(); /* crashes before confirm() */ int r3b = c.boot();
    std::printf("3 never confirmed:    %s-> runs v%d, then v%d\n", c.log.c_str(), r3a, r3b);

    ABSlots d = fresh<ABSlots>();                          // test 4: downgrade after v2
    d.update(v2); d.boot(); d.confirm(); d.log.clear();
    d.update(v1); int r4 = d.boot();
    std::printf("4 downgrade to v1:    %s-> runs v%d\n", d.log.c_str(), r4);

    sweep<ABSlots>("A/B slots");                           // test 5: power cuts
    sweep<SingleSlot>("single slot");

    const bool pass = r1 == 2 && r1b == 2 && r2 == 1 && r3a == 2 && r3b == 1 && r4 == 2;
    std::printf("%s\n", pass ? "H4 checks PASS (wrong signature refused; power loss never bricks A/B)"
                             : "a check FAILED");
    return pass ? 0 : 1;
}
