#ifndef EXIF_PARSER_H
#define EXIF_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// **** Error Handling **** //
typedef enum {
    ERR_OK = 0,
    ERR_EXIF_MISSING,
    ERR_ENDIAN_MISSING,
    ERR_TIFF_MISSING,
    ERR_INVALID_TAG,
    ERR_MALLOC_BYTE_TRANS,
    ERR_MALLOC_ASCII_TRANS,
    ERR_MALLOC_RATIONAL_TRANS,
    ERR_UNKNOWN,
} ErrorCode;

typedef struct {
    ErrorCode code;
    const char *message;
} Error;

const char *get_error_string(ErrorCode code);


// **** Exif Tag Interpreter **** //

// Lookup for Exif tags
typedef struct {
    uint16_t tag;
    const char *name;
} ExifTag;

const char *get_exif_tag_name(uint16_t tag);


// **** JPEG Processors **** //

void parse_jpeg(const uint8_t *buffer, size_t length);

static ErrorCode read_jpeg_u8(const uint8_t *buffer, size_t offset, size_t exifLength);

static ErrorCode translate_byte(uint8_t *input, size_t offset, const uint8_t *buffer, char *output);
static ErrorCode translate_ascii(const uint8_t *input, size_t offset, const uint8_t *buffer, char *output);


uint8_t read_u8(const uint8_t *buffer, size_t offset);
uint16_t read_u16_be(const uint8_t *buffer, size_t offset);
uint32_t read_u32_be(const uint8_t *buffer, size_t offset);


#endif // EXIF_PARSER_H