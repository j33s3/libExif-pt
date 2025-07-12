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
    ERR_TOO_SMALL,
    ERR_EXIF_MISSING,
    ERR_ENDIAN_MISSING,
    ERR_TIFF_MISSING,
    ERR_INVALID_TAG,
    ERR_MALLOC_BYTE_TRANS,
    ERR_MALLOC_ASCII_TRANS,
    ERR_MALLOC_SHORT_TRANS,
    ERR_MALLOC_LONG_TRANS,
    ERR_SHORT_COUNT,
    ERR_LONG_COUNT,
    ERR_RATIONAL_COUNT,
    ERR_MALLOC_RATIONAL_TRANS,
    ERR_MALLOC_UNDEFINED_TRANS,
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

/**
 * @brief Entry point for the parser
 * 
 * @param buffer image buffer, must malloc and free manually
 * @param length length of the buffer
 * @param outputBuffer output buffer, must malloc and free manually
 * @param outputCap output size
 */
void parse_jpeg(const uint8_t *buffer, size_t length, char *outputBuffer, size_t outputCap);



static ErrorCode read_jpeg_u8(const uint8_t *buffer, size_t offset, size_t exifLength, char *outputBuffer, size_t outputCap);

static ErrorCode translate_byte(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output);
static ErrorCode translate_ascii(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output);
static ErrorCode translate_rational(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output);
static ErrorCode translate_srational(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output);


#endif // EXIF_PARSER_H