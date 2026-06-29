#pragma once

#include <cstdint>
#include <fstream>

inline uint16_t swap16(uint16_t v) {
    return (v >> 8u) | (v << 8u);
}
inline uint32_t swap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
}

inline uint8_t readu1(std::ifstream& stream) {
    uint8_t result;
    stream.read(reinterpret_cast<char*>(&result), 1);
    return result;
}
inline uint16_t readu2(std::ifstream& stream) {
    uint16_t result;
    stream.read(reinterpret_cast<char*>(&result), 2);
    if constexpr (std::endian::native == std::endian::little)
        result = swap16(result);
    return result;
}
inline uint32_t readu4(std::ifstream& stream) {
    uint32_t result;
    stream.read(reinterpret_cast<char*>(&result), 4);
    if constexpr (std::endian::native == std::endian::little)
        result = swap32(result);
    return result;
}

