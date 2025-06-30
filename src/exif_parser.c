/*
 * @file            src/exif_parser.c
 * @description     Parses Exif data from an image file
 * @author          Jesse Peterson
 * @createTime      2025-06-27 22:51:55
 * @lastModified    2025-06-30 11:20:10
*/

#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "exif_parser.h"






const char *get_error_string(ErrorCode code) {
    switch (code) {
        case ERR_OK: return "No Error";
        case ERR_EXIF_MISSING: return "Missing EXIF Data";
        case ERR_ENDIAN_MISSING: return "Missing Enddianess";
        case ERR_TIFF_MISSING: return "Missing TIFF";
        case ERR_UNKNOWN: return "Unknown Error";
        default: return "unknown";
    }
}




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
    {0x9104, "ExposureCompensation"},
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


/**
 * @brief Get the exif tag name, if it does not exist then returns unkown
 * 
 * @param tag 
 * @return const char* 
 */
const char *get_exif_tag_name(uint16_t tag) {

    size_t count = sizeof(exif_tags) / sizeof(exif_tags[0]);
    for (size_t i = 0; i < 1; i++) {
        if (exif_tags[i].tag == tag) {
            return exif_tags[i].name;
        }
    }

    return "unknown";
}


/**
 * @brief Takes in a jpeg image as Uint8_t pointer and searches for the exif chunk. once found will pass the chunk to another function for parsing.
 * 
 * @param buffer Uint8_t pointer for the image
 * @param length Length of the image file
 */
void parse_jpeg(const uint8_t *buffer, size_t length) {
    size_t i = 2;
    uint16_t seg_length = 0;
    uint8_t offset = 0;

    // Iterate through the items till all is found
    while (i + 4 < length) {


        // If JFIF is present look for offset and skip
        if (((buffer[i] << 8) | buffer[i + 1]) == 0xFFE0) {
            i += ((buffer[i + 2] << 8) | buffer[i + 3]) + 1;
        }

        printf("%ld, ", i);

        // if the EXIF is present find the length and starting pointer
        if (((buffer[i] << 8) | buffer[i + 1]) == 0xFFE1) {

            seg_length = buffer[i+2] << 8 | buffer[i+3];

            if (i + 9 + seg_length <= length &&
                buffer[i + 4] == 'E' &&
                buffer[i + 5] == 'x' &&
                buffer[i + 6] == 'i' &&
                buffer[i + 7] == 'f' &&

                buffer[i + 8] == 0x00 &&
                buffer[i + 9] == 0x00
            ) {
                fflush(stdout);
                for(int j = i; j < seg_length + offset + 2; j++) {
                    printf("%02X,", buffer[j]);
                }
                fflush(stdout);

                ErrorCode response = read_jpeg_u8(buffer, offset, seg_length);

                // printf("ERROR: %s", get_error_string(response));
                printf("ERROR: %d ", response);
                printf("ERROR: %s", get_error_string(response));
                fflush(stdout);
            }


            break;
        }
        i++;
        // Tracks the offset from the begining of the image for reference later on
        offset = i;
    }

        printf("offset from the beggining, %d", offset);
        fflush(stdout);


        return;
}

/**
 * @brief Reads the exif data from the array
 * 
 * @param buffer 
 * @param offset 
 * @param exifLength 
 * @return uint8_t* 
 */
static ErrorCode read_jpeg_u8(const uint8_t *buffer, size_t offset, size_t exifLength) {
    char *outputBuffer[1001];
    int post = 0;
    uint16_t tiff_tags = 0;

    bool big_endian = false;

    // Iterator (Start at Exif and after Marker, seg_length and "Exif\0\0")
    size_t i = offset + 10;



    // Read the endianess
    switch((buffer[i] << 8) | buffer[i + 1]) {
        case(0x4D4D): 
            big_endian = true;
            break;
        case(0x4949):
            big_endian = false;
            break;
        default: 
            return ERR_ENDIAN_MISSING;
    } 
    i += 2;

    // Read the TIFF magic number "42"
    if(((buffer[i] << 8) | buffer[i + 1]) != 0x002A) {
        return ERR_TIFF_MISSING;
    }
    i += 2;

    // skip over the offset to first IFD
    i += 4;

    // Read the number of TIFF tags
    tiff_tags = ((buffer[i] << 8) | buffer[i + 1]);
    i += 2;

    printf("\n TIFF TAGS %d\n", tiff_tags);

    
    // TODO ADD OTHER TAGS HERE
    printf("%s", get_exif_tag_name((buffer[i] << 8) | buffer[i + 1]));

    return ERR_OK;
    

    


}


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
