#ifndef LIBFFSHIT_PATTERNS_PATTERN_H
#define LIBFFSHIT_PATTERNS_PATTERN_H

#include <vector>
#include <sstream>

#include <fmt/format.h>

#include "ffshit/patterns/raw.h"
#include "ffshit/patterns/ex.h"

namespace Patterns {

using Readable = std::vector<std::string>;

template<typename T>
class Pattern {
    static_assert(  !std::is_same<uint64_t, T>::value ||
                    !std::is_same<uint32_t, T>::value ||
                    !std::is_same<uint16_t, T>::value ||
                    !std::is_same<uint8_t, T>::value, "T must be uint64_t uint32_t or uint16_t or uint8_t");

    public:
        Pattern(const Readable &readable) {
            this->readable = readable;

            constexpr size_t align = sizeof(T);

            auto hex_to_T = [](const std::string &h) -> T {
                std::stringstream   ss;
                uint32_t            out;

                ss << std::hex << h;
                ss >> out;

                return out;
            };

            for (const auto &uint32_str : readable) {
                std::vector<std::string> bytes;

                std::stringstream   ss(uint32_str);
                std::string         byte;

                while (std::getline(ss, byte, ' ')) {
                    bytes.push_back(byte);
                }

                if (bytes.size() != align) {
                    throw Exception("Broken pattern");
                }

                std::reverse(bytes.begin(), bytes.end());

                std::string data_value_str;
                std::string data_mask_str;

                for (size_t i = 0; i < align; ++i) {
                    std::string byte_str = bytes[i];
            
                    patter_str += fmt::format("{}", byte_str);

                    for (const auto &c : byte_str) {
                        if (c == '?') {
                            data_value_str.push_back('0');
                            data_mask_str.push_back('0');
                        } else {
                            data_value_str.push_back(c);
                            data_mask_str.push_back('F');
                        }
                    }
                }

                T data_value = hex_to_T(data_value_str);
                T data_mask  = hex_to_T(data_mask_str);

                raw.emplace_back(RawPart<T>(data_value, data_mask));

                patter_str += " ";

            }
        };

        std::string to_string() {
            return patter_str;
        }

        const Raw<T> &  get_raw() {
            return raw;
        }

        bool match(T *data_ptr) {
            size_t offset = 0;

            for (const auto &raw_part : raw) {
                const T &data_word  = *(data_ptr + offset);

                if (!raw_part.match(data_word)) {
                    return false;
                }

                ++offset;
            }

            return true;
        }


    private:
        Raw<T>          raw;
        Readable        readable;

        std::string     patter_str;
};

};

#endif
