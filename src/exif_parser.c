#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "exif_parser.h"


// E
void parse_jpeg(const uint8_t *buffer, size_t length) {
    size_t i = 2;

    while (i + 4 < length) {

        // iterate till the Exif data is found
        if (((buffer[i] << 8) | buffer[i + 1]) == 0xFFE1) {
            // Read the segment length
            uint16_t seg_length = buffer[i+2] << 8 | buffer[i+3];

            if (i + 9 + seg_length <= length &&
                buffer[i + 4] == 'E' &&
                buffer[i + 5] == 'x' &&
                buffer[i + 6] == 'i' &&
                buffer[i + 7] == 'f' &&

                buffer[i + 8] == 0x00 &&
                buffer[i + 9] == 0x00
            ) {
                printf("Length: %d\n\n", seg_length - 8);
                
                for(int i = 0; i < seg_length; i++) {
                    printf("%d,", buffer[i + 10]);
                }

                fflush(stdout);
            }
            
        }
    }
}

// static read_jpeg_u8()


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
