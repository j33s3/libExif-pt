#ifndef FORMAT_READER_H
#define FORMAT_READER_H


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>



//////// ** ////////
// FORMAT READERS //
//////// ** ////////

bool is_jpeg(const uint8_t *buffer, size_t length);

bool is_png();

bool is_avif();

bool is_heic();

bool is_webp();

uint8_t readImageFormat(const uint8_t *buffer, size_t length);

#endif // FORMAT_READER_H