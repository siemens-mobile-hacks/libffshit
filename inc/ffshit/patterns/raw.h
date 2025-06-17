#ifndef LIBFFSHIT_PATTERNS_RAW_H
#define LIBFFSHIT_PATTERNS_RAW_H

#include <cstdint>
#include <type_traits>

namespace Patterns {

template<typename T>
class RawPart {
    static_assert(  !std::is_same<uint64_t, T>::value ||
                    !std::is_same<uint32_t, T>::value ||
                    !std::is_same<uint16_t, T>::value ||
                    !std::is_same<uint8_t, T>::value, "T must be uint64_t uint32_t or uint16_t or uint8_t");

    public:
        RawPart(RawPart &&prev) = default;
        RawPart(const RawPart &prev) = default;

        RawPart(T value, T mask) : value(value), mask(mask) { }

        T get_value() {
            return value;
        }

        T get_mask() {
            return mask;
        }

        bool match(const T &data) const {
            return (data & mask) == value;
        }

    private:
        uint32_t value;
        uint32_t mask;
};

template <typename T>
using Raw = std::vector<RawPart<T>>;

};

#endif
