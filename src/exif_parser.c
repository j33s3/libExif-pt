/*
 * @file            src/exif_parser.c
 * @description     Parses Exif data from an image file
 * @author          Jesse Peterson
 * @createTime      2025-06-27 22:51:55
 * @lastModified    2025-06-30 11:20:10
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "exif_parser.h"






const char *get_error_string(ErrorCode code) {
    switch (code) {
        case ERR_OK: return "No Error";
        case ERR_EXIF_MISSING: return "Missing EXIF Data";
        case ERR_ENDIAN_MISSING: return "Missing Enddianess";
        case ERR_TIFF_MISSING: return "Missing TIFF";
        case ERR_INVALID_TAG: return "Tag is unkown or invalid";
        case ERR_MALLOC_BYTE_TRANS: return "Error allocating memory when translating from byte";
        case ERR_MALLOC_ASCII_TRANS: return "Error allocating memory when translating from ascii";
        case ERR_SHORT_COUNT: return "The count of short items exceeds 1";
        case ERR_LONG_COUNT: return "The count of long items exceeds 4";
        case ERR_RATIONAL_COUNT: return "The count of rational items exceeds 1";
        case ERR_MALLOC_RATIONAL_TRANS: return "Error allocating memory when translating from rational";
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
    uint8_t offset = 10;

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

                fflush(stdout);
            }


            break;
        }
        i++;
        // Tracks the offset from the begining of the image for reference later on
        offset += i;
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
    int pos = 0;
    uint16_t tiff_tags = 0;

    bool big_endian = false;

    // Iterator (Start at Exif and after Marker, seg_length and "Exif\0\0")
    size_t i = offset;



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
    // Each TIFF item is going to be 12 bytes
    for(int j = 0; j < tiff_tags; j++) {
        const char *tagName = get_exif_tag_name((buffer[i] << 8) | buffer[i + 1]);
        i += 2;
        
        uint8_t input[8];

        for (int j = 0; j < 8; j++) {
            input[j] = buffer[i + 2 + j];
        }

        char *output;

        ErrorCode status;

        // get the byte count from tag
        const uint32_t count = ((input[0]) << 24 |
                                (input[1]) << 16 | 
                                (input[2]) << 8 |
                                (input[3]));

        switch ((buffer[i] << 8) | buffer[i + 1]) {
            case 0x0001: //byte
                printf("\n%s\n", "byte");
                status = translate_byte(input, offset, buffer, output);
                break;
            case 0x0002: //ascii
                printf("\n%s\n", "ascii");
                status = translate_ascii(input, offset, buffer, output);
                break;
            case 0x0003:  //short
                printf("\n%s\n", "short");



                if (count > 1) {
                    return ERR_SHORT_COUNT;
                }

                // Adding the item
                snprintf(&output[0], sizeof(output), "%d", (int16_t)((input[4] << 8) | input[5]));
                
                break;
            case 0x0004: //long
                printf("\n%s\n", "long");


                if (count > 4) {
                    return ERR_LONG_COUNT;
                }

                // Adding the item
                snprintf(&output[0], sizeof(output), "%d", (int32_t)((
                    input[4] << 24 | 
                    input[5] << 16 |
                    input[6] << 8  | 
                    input[7])
                ));

                break;
            case 0x0005: // rational
                printf("\n%s\n", "rational");
                status = translate_rational(input, offset, buffer, output);
                break;
            case 0x0007: //undefined
                // TODO
                output = "undefined";
                break;
            case 0x0009: //slong
                output = "undefined";
                // status = translate_slong(input, offset, buffer);
                break;
            case 0x000A: //srational
                // status = translate_srational(input, offset, buffer);
                output = "undefined";
                break;
        default: return ERR_INVALID_TAG;

        i += 10;
    }

    // todo convert to user error
    if (get_error_string(status)) {
        int len = strlen(output);

        if(pos + len < sizeof(outputBuffer)) {
            strcpy(&outputBuffer[pos], output);
        }
        free(output);  // clean up!
    }




    }
    printf("%s", get_exif_tag_name((buffer[i] << 8) | buffer[i + 1]));

    return ERR_OK;
    
}

static ErrorCode translate_byte(uint8_t *input, size_t offset, const uint8_t *buffer, char *output) {
    // iterator
    size_t i = 0;

    // get the byte count from tag
    const uint32_t count = ((input[i]) << 24 |
                            (input[i + 1]) << 16 | 
                            (input[i + 2]) << 8 |
                            (input[i + 3]));
    i += 4;

    // fetches the value from the tag
    const uint32_t value = ((input[i]) << 24 |
                        (input[i + 1]) << 16 | 
                        (input[i + 2]) << 8 |
                        (input[i + 3]));
    
    // if the count is under 4 bytes look at tag
    if (count <= 4) {

        output = malloc(11);
        if (output == NULL) {
            return ERR_MALLOC_BYTE_TRANS;
        }

        snprintf(output, 11, "0x%08X", value);

        // Success
        return ERR_OK;

    // if the count is too large look at offset
    } else {
        char *output = malloc(count + 3);
        int pos = 0;
        
        // if output did not allocate properly
        if (output == NULL) {
            return ERR_MALLOC_BYTE_TRANS;
        }

        // itterate over every byte and append it to the output buffer
        for (int j = offset + value; j < count + offset + value; j++) {
            int written = snprintf(&output[pos], sizeof(output) - pos, "0x%02X ", i);
            if (written < 0 || written >= (int)(sizeof(output) - pos)) {
                break;
            }
            pos += written;
        }

        // Success
        return ERR_OK;
    }

}

static ErrorCode translate_ascii(const uint8_t *input, size_t offset, const uint8_t *buffer, char *output) {
    // Iterator
    size_t i = 0;



        // get the byte count from tag
    const uint32_t count = ((input[i]) << 24 |
                            (input[i + 1]) << 16 | 
                            (input[i + 2]) << 8 |
                            (input[i + 3]));

    i += 4;

        printf("\n%d\n", i);




    // less than 4 bytes
    if (count <= 4) {



        

        // try to allocate memory to the output
        output = malloc(11);
        if(output == NULL) {
            return ERR_MALLOC_ASCII_TRANS;
        }

    size_t pos = 0;

    for (int j = i; j < count; j++) {
        char c = (char)input[j];
        if (isprint(c)) {
            output[pos++] = c;
        } else {
            output[pos++] = '.';  // replace non-printables
        }
    }

    output[pos] = '\0';  // null-terminate


    }

    // exceeds 4 bytes
    else {



        output = malloc(count + 3);

        
        if(output == NULL) {
            return ERR_MALLOC_ASCII_TRANS;
        }

        size_t pos = 0;





        // fetches the value from the tag
        const uint32_t value = ((input[i]) << 24 |
                            (input[i + 1]) << 16 | 
                            (input[i + 2]) << 8 |
                            (input[i + 3]));




        for (int j = offset + value; j < offset + value + count; j++) {

        printf("\n%d,", j - offset - value, buffer[j]);


            char c = (char)buffer[j];
            printf("\n%c\n", c);
            if (isprint(c)) {
                output[pos++] = c;
            } else {
                output[pos++] = '.';  // replace non-printables
            }
        }
        output[pos] = '\0';
    }

    // Success
    return ERR_OK;
}



static ErrorCode translate_rational(const uint8_t *input, size_t offset, const uint8_t *buffer, char *output) {
    // iterator

    // get the byte count from tag
    const uint32_t count = ((input[0]) << 24 |
                            (input[1]) << 16 | 
                            (input[2]) << 8 |
                            (input[3]));

    // fetches the value from the tag
    const uint32_t value = ((input[4]) << 24 |
                        (input[5]) << 16 | 
                        (input[6]) << 8 |
                        (input[7]));

    // Check that the count is only one rational
    if (count != 1) {
       return ERR_RATIONAL_COUNT;
    }

    size_t dataLoc = offset + value;

    // Read the values from the resgister
    const uint32_t numerator = (buffer[dataLoc++] << 24 |
                                buffer[dataLoc++] << 16 |
                                buffer[dataLoc++] << 8 |
                                buffer[dataLoc++]);

    const uint32_t denominator = (buffer[dataLoc++] << 24 |
                                  buffer[dataLoc++] << 16 |
                                  buffer[dataLoc++] << 8 |
                                  buffer[dataLoc++]);

    // convert the data into a string
    snprintf(&output[0], sizeof(output), "%d/%d", numerator, denominator);


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
