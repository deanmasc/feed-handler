#ifndef READ_DATA_TYPES_H
#define READ_DATA_TYPES_H

#include <cstring>      // memcpy
#include <cstdint>      // uintN_t
#include <cstddef>      // size_t
#include <array>        // std::array
#include <arpa/inet.h>  // ntohs, ntohl

// ---- Field readers -------------------------------------------------------
// Each reader copies a big-endian field out of msg at the current offset,
// converts it to host byte order, and advances offset past it.

// Reads a 2, 4, or 8 byte unsigned integer. The correct byte-swap is chosen
// at compile time from the type's size, so one function covers all of them.
template <typename T>
void read_int(T& out, int& offset, const char* msg) {
    memcpy(&out, msg + offset, sizeof(T));
    if constexpr (sizeof(T) == 2)      out = ntohs(out);
    else if constexpr (sizeof(T) == 4) out = ntohl(out);
    else if constexpr (sizeof(T) == 8) out = __builtin_bswap64(out);
    offset += sizeof(T);
}

// Reads a fixed-width std::array field (e.g. 8-byte symbol). Not null-terminated.
template <size_t N>
inline void read_array(std::array<char, N>& arr, int& offset, const char* msg) {
    memcpy(arr.data(), msg + offset, N);
    offset += N;
}

// Reads the ITCH 6-byte (48-bit) big-endian timestamp into a uint64_t.
// The 6 bytes are copied into the high end so a full 8-byte swap lands them correctly.
inline void read_timestamp(uint64_t& out, int& offset, const char* msg) {
    out = 0;
    memcpy(reinterpret_cast<char*>(&out) + 2, msg + offset, 6);
    out = __builtin_bswap64(out);
    offset += 6;
}

// Reads a single alpha byte (e.g. side, event_code). No byte order concerns.
inline void read_char(char& out, int& offset, const char* msg) {
    out = msg[offset];
    ++offset;
}

// Reads the header shared by every message: stock_locate (1/2), tracking_num (3/2),
// timestamp (5/6). Leaves offset at the first type-specific field.
inline void read_header(uint16_t& stock_locate, uint16_t& tracking_num, uint64_t& timestamp,
                 int& offset, const char* msg) {
    read_int(stock_locate, offset, msg);
    read_int(tracking_num, offset, msg);
    read_timestamp(timestamp, offset, msg);
}
// -------------------------------------------------------------------------

#endif // READ_DATA_TYPES_H