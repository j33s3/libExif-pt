#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "exif_parser.h"





uint8_t readImageFormat(const uint8_t *buffer, size_t length) {
    r

}

bool is_jpeg(const uint8_t *buffer, size_t length) {

    const uint16_t SOI = 0xFFD8;
    const uint16_t EOI = 0xFFD9;


    // check first two bytes as jpeg start of image
    if (((buffer[0] << 8) | buffer[1]) != SOI) {
        return false;
    }

    // check last two bytes as jpeg end of image
    if(((buffer[length - 2] << 8) | buffer[length - 1]) != EOI) {
        return false;
    }


    // File is jpeg
    return true;

            // TODO check these
    // 1. walk through each segment (marker starts with 0xFF)
    // 2. handle segment lengths
    // 3. skip or parse APPO/APP1 segments
    // 4. ensure there's at least one SOS and one EOI marker
    // 5. Validate no bad lengths or crashes

}

bool is_png() {
    return false;
}

bool is_avif() {
    return false;
}

bool is_heic() {
    return false;
}

bool is_webp() {
    return false;
}
