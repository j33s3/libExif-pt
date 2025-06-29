#include <stdio.h>
#include <stdint.h>
#include "exif_parser.h"

int main() {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    printf("U8: %u\n", read_u8(data, 0));
    printf("U16: %u\n", read_u16_be(data, 0));
    printf("U32: %u\n", read_u32_be(data, 0));

    return 0;
}
