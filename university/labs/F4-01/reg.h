// reg.h - DR301: a typed register-access layer.
// A register's offset, width and access rule are part of its type, so a wrong-width access
// or a write to a read-only register is a compile error, not a silent bug on hardware.
// The layer does not know how the bus is reached: a Bus type supplies read<T>/write<T>.
#pragma once
#include <stdint.h>

enum class Access { RO, WO, RW, RW1C };   // RW1C: write 1 to clear a bit, 0 leaves it

template <typename T, uint32_t Offset, Access A>
struct Reg {
    using type = T;
    static constexpr uint32_t offset = Offset;
    static constexpr Access access = A;
};

// A bit field inside a register: [Shift, Shift + Width).
template <typename R, unsigned Shift, unsigned Width>
struct Field {
    using reg = R;
    using T = typename R::type;
    static_assert(Width >= 1 && Shift + Width <= sizeof(T) * 8, "field outside the register");
    static constexpr T mask = static_cast<T>(((Width == sizeof(T) * 8) ? ~T{0} : ((T{1} << Width) - 1)) << Shift);
    static constexpr T get(T v) { return static_cast<T>((v & mask) >> Shift); }
    static constexpr T put(T v, T x) { return static_cast<T>((v & ~mask) | ((x << Shift) & mask)); }
};

template <typename Bus>
class RegBlock {
public:
    explicit RegBlock(Bus bus) : bus_(bus) {}

    template <typename R> typename R::type read() const
    {
        static_assert(R::access != Access::WO, "this register is write-only");
        return bus_.template read<typename R::type>(R::offset);
    }
    template <typename R> void write(typename R::type v)
    {
        static_assert(R::access == Access::RW || R::access == Access::WO,
                      "this register cannot be written with write(); see clear() for RW1C");
        bus_.template write<typename R::type>(R::offset, v);
    }
    // Write-1-to-clear: only the bits that are 1 in 'bits' are cleared.
    template <typename R> void clear(typename R::type bits)
    {
        static_assert(R::access == Access::RW1C, "clear() is only for write-1-to-clear registers");
        bus_.template write<typename R::type>(R::offset, bits);
    }
    // Read-modify-write of one field; the other bits are written back unchanged.
    template <typename F> void set_field(typename F::T x)
    {
        using R = typename F::reg;
        write<R>(F::put(read<R>(), x));
    }
    template <typename F> typename F::T get_field() const { return F::get(read<typename F::reg>()); }

private:
    Bus bus_;
};

// Memory-mapped registers: volatile accesses at base + offset.
struct MmioBus {
    uintptr_t base;
    template <typename T> T read(uint32_t off) const
    {
        return *reinterpret_cast<volatile T*>(base + off);
    }
    template <typename T> void write(uint32_t off, T v) const
    {
        *reinterpret_cast<volatile T*>(base + off) = v;
    }
};
