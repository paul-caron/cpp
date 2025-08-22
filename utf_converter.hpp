#ifndef UTF_CONVERTER_HPP
#define UTF_CONVERTER_HPP

#include <string>
#include <stdexcept>
#include <vector>

namespace utf_converter {

    // Converts a UTF-8 encoded std::string to a UTF-32 encoded std::u32string
    inline std::u32string utf8_to_utf32(const std::string& utf8_str) {
        std::u32string result;
        result.reserve(utf8_str.size()); // Pre-allocate for efficiency

        for (size_t i = 0; i < utf8_str.size(); ) {
            unsigned char c = static_cast<unsigned char>(utf8_str[i]);
            char32_t code_point = 0;

            if (c <= 0x7F) { // 1-byte sequence (ASCII)
                code_point = c;
                i += 1;
            } else if ((c & 0xE0) == 0xC0) { // 2-byte sequence
                if (i + 1 >= utf8_str.size()) {
                    throw std::runtime_error("Invalid UTF-8: incomplete 2-byte sequence");
                }
                unsigned char c2 = static_cast<unsigned char>(utf8_str[i + 1]);
                if ((c2 & 0xC0) != 0x80) {
                    throw std::runtime_error("Invalid UTF-8: invalid continuation byte in 2-byte sequence");
                }
                code_point = (c & 0x1F) << 6;
                code_point |= (c2 & 0x3F);
                if (code_point < 0x80) { // Overlong encoding
                    throw std::runtime_error("Invalid UTF-8: overlong 2-byte sequence");
                }
                i += 2;
            } else if ((c & 0xF0) == 0xE0) { // 3-byte sequence
                if (i + 2 >= utf8_str.size()) {
                    throw std::runtime_error("Invalid UTF-8: incomplete 3-byte sequence");
                }
                unsigned char c2 = static_cast<unsigned char>(utf8_str[i + 1]);
                unsigned char c3 = static_cast<unsigned char>(utf8_str[i + 2]);
                if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) {
                    throw std::runtime_error("Invalid UTF-8: invalid continuation byte in 3-byte sequence");
                }
                code_point = (c & 0x0F) << 12;
                code_point |= (c2 & 0x3F) << 6;
                code_point |= (c3 & 0x3F);
                if (code_point < 0x800) { // Overlong encoding
                    throw std::runtime_error("Invalid UTF-8: overlong 3-byte sequence");
                }
                i += 3;
            } else if ((c & 0xF8) == 0xF0) { // 4-byte sequence
                if (i + 3 >= utf8_str.size()) {
                    throw std::runtime_error("Invalid UTF-8: incomplete 4-byte sequence");
                }
                unsigned char c2 = static_cast<unsigned char>(utf8_str[i + 1]);
                unsigned char c3 = static_cast<unsigned char>(utf8_str[i + 2]);
                unsigned char c4 = static_cast<unsigned char>(utf8_str[i + 3]);
                if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) {
                    throw std::runtime_error("Invalid UTF-8: invalid continuation byte in 4-byte sequence");
                }
                code_point = (c & 0x07) << 18;
                code_point |= (c2 & 0x3F) << 12;
                code_point |= (c3 & 0x3F) << 6;
                code_point |= (c4 & 0x3F);
                if (code_point < 0x10000) { // Overlong encoding
                    throw std::runtime_error("Invalid UTF-8: overlong 4-byte sequence");
                }
                i += 4;
            } else {
                throw std::runtime_error("Invalid UTF-8: unrecognized byte sequence");
            }

            // Validate code point
            if (code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF)) {
                throw std::runtime_error("Invalid UTF-8: invalid code point");
            }

            result.push_back(code_point);
        }

        return result;
    }

    // Converts a UTF-32 encoded std::u32string to a UTF-8 encoded std::string
    inline std::string u32_to_utf8(const std::u32string& utf32_str) {
        std::string utf8;
        for (char32_t cp : utf32_str) {
            // Validate code point
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                throw std::runtime_error("Invalid UTF-32: invalid code point");
            }

            if (cp < 0x80) {
                utf8 += static_cast<char>(cp);
            } else if (cp < 0x800) {
                utf8 += static_cast<char>(0xC0 | (cp >> 6));
                utf8 += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                utf8 += static_cast<char>(0xE0 | (cp >> 12));
                utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                utf8 += static_cast<char>(0xF0 | (cp >> 18));
                utf8 += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        return utf8;
    }

} // namespace utf_converter



#endif
