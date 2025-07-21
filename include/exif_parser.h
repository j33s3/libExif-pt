/*
 * @file            include/exif_parser.h
 * @description
 * @author          Jesse Peterson
 * @createTime      2025-06-27 22:51:55
 * @lastModified    2025-07-18 09:32:31
 */

#ifndef EXIF_PARSER_H
#define EXIF_PARSER_H

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// **** Error Handling **** //
typedef enum {
  ERR_OK = 0,
  ERR_TOO_SMALL,
  ERR_EXIF_MISSING,
  ERR_TIFF_OVERFLOW,
  ERR_EXIF_OVERFLOW,
  ERR_ENDIAN_MISSING,
  ERR_TIFF_MISSING,
  ERR_INVALID_TAG,
  ERR_MALLOC,
  ERR_SHORT_COUNT,
  ERR_LONG_COUNT,
  ERR_RATIONAL_COUNT,
  ERR_UNKNOWN_UNDEFINED,
  ERR_UNKNOWN,
} ErrorCode;

typedef struct {
  ErrorCode code;
  const char *message;
} Error;

// **** Exif Parser **** //

// Lookup for Exif tags
typedef struct {
  uint16_t tag;
  const char *name;
} ExifTag;

// Array of ExifTag struct
static const ExifTag exif_tags[] = {
    {0x010F, "Make"},
    {0x0110, "Model"},
    {0x0112, "Orientation"},
    {0x011A, "XResolution"},
    {0x011B, "YResolution"},
    {0x0128, "ResolutionUnit"},
    {0x0131, "Software"},
    {0x0132, "ModifyDate"},
    {0x8298, "Copyright"},
    {0x8769, "ExifOffset"},

    {0x829A, "ExosureTime"},
    {0x829D, "FNumber"},
    {0x8822, "ExposureProgram"},
    {0x8827, "ISO"},
    {0x8830, "SensitivityType"},
    {0x8831, "StandardOutputSensitivity"},
    {0x9000, "ExifVersion"},
    {0x9003, "DateTimeOriginal"},
    {0x9004, "CreateDate"},
    {0x9101, "ComponentsConfiguration"},
    {0x9204, "ExposureCompensation"},
    {0x9207, "MeteringMode"},
    {0x9209, "Flash"},
    {0x920A, "FocalLength"},
    {0xA000, "FlashpixVersion"},
    {0xA001, "ColorSpace"},
    {0xA002, "ExifImageWidth"},
    {0xA003, "ExifImageHeight"},
    {0xA217, "SensingMethod"},
    {0xA300, "FileSource"},
    {0xA301, "SceneType"},
    {0xA401, "CustomRendered"},
    {0xA402, "ExposureMode"},
    {0xA403, "WhiteBalanced"},
    {0xA405, "FocalLengthIn35mmFormat"},
    {0xA406, "SceneCaptureType"},
    {0xA408, "Contrast"},
    {0xA409, "Saturation"},
    {0xA40A, "Sharpness"},
    {0xA40C, "SubjectDistanceRange"},
    {0xA500, "Gamma"},
};


// ** Entry point ** //
char *parse_jpeg(const uint8_t *buffer, size_t length);

// **** STATIC FUNCTIONS **** //

// ** Helper Functions ** //
static char *get_error_string(ErrorCode code);
static const char *get_exif_tag_name(uint16_t tag);

// ** Parsing functions ** //
static ErrorCode u8_crawler(const uint8_t *buffer, uint16_t seg_length, size_t offset, char **output);
static ErrorCode translate_byte(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);
static ErrorCode translate_ascii(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);
static ErrorCode translate_short(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian, const uint16_t tag);
static ErrorCode translate_long(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);
static ErrorCode translate_rational(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);
static ErrorCode translate_undefined(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian, const uint16_t tag);
static ErrorCode translate_slong(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);
static ErrorCode translate_srational(const uint8_t *buffer, const uint32_t count, const uint8_t *val_or_off, size_t offset, char **response, const bool big_endian);


#endif // EXIF_PARSER_H
