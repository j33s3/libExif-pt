#ifndef EXIF_PARSER_H
#define EXIF_PARSER_H

#include <stdint.h>
#include <stddef.h>


void parse_jpeg(const uint8_t *buffer, size_t length);

uint8_t read_u8(const uint8_t *buffer, size_t offset);
uint16_t read_u16_be(const uint8_t *buffer, size_t offset);
uint32_t read_u32_be(const uint8_t *buffer, size_t offset);


#endif // EXIF_PARSER_H