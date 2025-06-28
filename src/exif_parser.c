#include <stdint.h>
#include <stddef.h>
#include "exif_parser.h"


uint8_t read_u8(const uint8_t *buffer, size_t offset) {
    return buffer[offset];
}

uint16_t read_u16_be(const uint8_t *buffer, size_t offset) {
    return (buffer[offset] << 8) | buffer[offset + 1];
}

uint32_t read_u32_be(const uint8_t *buffer, size_t offset) {
    return (buffer[offset] << 24) |
           (buffer[offset + 1] << 16) |
           (buffer[offset + 2] << 8) |
           (buffer[offset + 3]);
}
