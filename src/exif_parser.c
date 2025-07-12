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
        case ERR_TOO_SMALL: return "Output Buffer is too small";
        case ERR_EXIF_MISSING: return "Missing EXIF Data";
        case ERR_ENDIAN_MISSING: return "Missing Enddianess";
        case ERR_TIFF_MISSING: return "Missing TIFF";
        case ERR_INVALID_TAG: return "Tag is unkown or invalid";
        case ERR_MALLOC_BYTE_TRANS: return "Error allocating memory when translating from byte";
        case ERR_MALLOC_ASCII_TRANS: return "Error allocating memory when translating from ascii";
        case ERR_MALLOC_SHORT_TRANS: return "Error allocating memory when translating from short";
        case ERR_MALLOC_LONG_TRANS: return "Error allocating memory when translating from long or signed";
        case ERR_SHORT_COUNT: return "The count of short items exceeds 1";
        case ERR_LONG_COUNT: return "The count of long items exceeds 4";
        case ERR_RATIONAL_COUNT: return "The count of rational items exceeds 1";
        case ERR_MALLOC_RATIONAL_TRANS: return "Error allocating memory when translating from rational or signed rational";
        case ERR_MALLOC_UNDEFINED_TRANS: return "Error allocating memory when translating from undefined value";
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


/**
 * @brief Get the exif tag name, if it does not exist then returns unkown
 * 
 * @param tag 
 * @return const char* 
 */
const char *get_exif_tag_name(uint16_t tag) {

    size_t count = sizeof(exif_tags) / sizeof(exif_tags[0]);
    for (size_t i = 0; i < count; i++) {
        if(exif_tags[i].tag == tag) {
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
void parse_jpeg(const uint8_t *buffer, size_t length, char *outputBuffer, size_t outputCap) {
    size_t i = 2;
    uint16_t seg_length = 0;
    uint8_t offset = 10;

    // Check Malloc is processed
    if (outputBuffer == NULL) {
        return;
    }

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



                // fflush(stdout);
                // for(int j = i; j < seg_length + offset + 2; j++) {
                //     printf("%02X,", buffer[j]);
                // }
                // fflush(stdout);

                ErrorCode response = read_jpeg_u8(buffer, offset, seg_length, outputBuffer, outputCap);

                // fflush(stdout);
            }


            break;
        }
        i++;
        // Tracks the offset from the begining of the image for reference later on
        offset += i;
    }

        printf("%s", "EOF");
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
static ErrorCode read_jpeg_u8(const uint8_t *buffer, size_t offset, size_t exifLength, char *outputBuffer, size_t outputCap) {

    int pos = 0;
    outputBuffer[pos++] = '{';
    uint16_t tiff_tags = 0;
    uint16_t exif_tags = 0;

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


    fflush(stdout);
    printf("\n TIFF TAGS %d\n", tiff_tags);

    

    // Each TIFF item is going to be 12 bytes
    for(int j = 0; j < tiff_tags + exif_tags; j++) {

        // ** TAG ** //
        const  uint16_t tag = (buffer[i] << 8) | buffer[i + 1];
        i += 2;

        // ** TYPE ** //
        const uint16_t type = ((buffer[i] << 8) | buffer[i + 1]);
        i += 2;
        

        // ** COUNT ** //
        const uint32_t count = ((buffer[i]) << 24 |
                                (buffer[i + 1]) << 16 |
                                (buffer[i + 2]) << 8  |
                                (buffer[i + 3]));
        i += 4;

        // ** VALUE ** //
        uint8_t value[20];
        for (int k = 0; k < 4; k++) {
            value[k] = buffer[i++];
        }

        // store output and accept respounse as ErrorCode
        char *output;
        ErrorCode status;





        switch (type) {
            // **** BYTE **** //
            case 0x0001: 
                output = malloc(count + 1);
                status = translate_byte(count, value, offset, buffer, output);
                break;

            // **** ASCII **** //
            case 0x0002:
                output = malloc(count + 3);
                if(output == NULL) {
                    status = ERR_MALLOC_ASCII_TRANS;
                    break;
                }

                status = translate_ascii(count, value, offset, buffer, output);


                break;

            // **** SHORT **** //
            case 0x0003: //short

                // to accomadate for short max = 65535
                output = malloc(15);


                if(output == NULL) {
                    status = ERR_MALLOC_SHORT_TRANS;
                    break;
                }

                if (count > 1) {
                    status = ERR_SHORT_COUNT;
                }
                else if (tag == 0xA001){
                    switch((value[0] << 8) | value[1]) {
                        case 0x1:
                            snprintf(&output[0], 15, "%s", "sRGB");
                            break;
                        case 0x2:
                            snprintf(&output[0], 15, "%s", "Adobe RGB");
                            break;
                        case 0xFFFD:
                            snprintf(&output[0], 15, "%s", "Wide Gamut RGB");
                            break;
                        case 0xFFFE:
                            snprintf(&output[0], 15, "%s", "ICC Profile");
                            break;
                        case 0xFFFF:
                            snprintf(&output[0], 15, "%s", "Uncalibrated");
                            break;
                        default: 
                            snprintf(&output[0], 15, "%s", "Unknown");
                            break;
                    }
                }
                else {
                    // Adding the itemd
                    snprintf(&output[0], sizeof(output), "%d", (uint16_t)((value[0] << 8) | value[1]));
                    status = ERR_OK;
                }

                break;

            // **** LONG **** //
            case 0x0004:
                
                // to accomadate for long max = 4294967295
                output = malloc(15);
                if(output == NULL) {
                    status = ERR_MALLOC_LONG_TRANS;
                }
                // if error in the count
                if (count > 1) {
                    status = ERR_LONG_COUNT;
                    break;
                }

                // ** EXIF OFFSET ** //
                else if (exif_tags == 0 && tag == 0x8769) {
                    i = offset +    ((value[0] << 24) |
                                     (value[1] << 16) |
                                     (value[2] << 8) |
                                     (value[3]));

                    exif_tags = ((buffer[i] << 8) | buffer[i + 1]);
                    i += 2;
                }

                // ** LONG ** //
                else {
                    // Adding the item
                    snprintf(&output[0], sizeof(output), "%d", (uint32_t)((
                        value[0] << 24 | 
                        value[1] << 16 |
                        value[2] << 8  |
                        value[3])
                    ));
                    status = ERR_OK;
                }


                break;

            // **** RATIONAL **** //
            case 0x0005: // rational
                output = malloc(30);
                if(output == NULL) {
                    status = ERR_MALLOC_RATIONAL_TRANS;
                }
            
                status = translate_rational(count, value, offset, buffer, output);
                break;

            // **** UNDEFINED **** //
            case 0x0007: //undefined

                size_t outPos = 0;

                switch(tag) {
                    // Itterator for values

                    case 0x9000:    // Exif Version
                    case 0xA000:    // FlashpixVersion
                        output = malloc(10);

                        if (output == NULL) {
                            status = ERR_MALLOC_UNDEFINED_TRANS; 
                            break;
                        }

                        if (count > 4) {
                            snprintf(&output[0], 10, "%s", "Unknown");
                            break;
                        }

                        for(int it = 0; it < 4; it++) {
                            char c = (char)value[it];
                            output[outPos++] = c;
                            if(outPos == 2) {
                                output[outPos++] = '.';
                            }
                        }
                        status = ERR_OK;

                        break;
                    case 0x9101:    // ComponentConfiguration
                        output = malloc(10);

                        if (output == NULL) {
                            status = ERR_MALLOC_UNDEFINED_TRANS; 
                            break;
                        }

                        if (count > 4) {
                            snprintf(&output[0], 10, "%s", "Unknown");
                            break;
                        }

                        for (int it = 0; it < 4; it++) {
                            switch(value[it]) {
                                case 0:
                                    output[outPos++] = '-';
                                    break;
                                case 1:
                                    output[outPos++] = 'Y';
                                    break;
                                case 2:
                                    output[outPos++] = 'C';
                                    output[outPos++] = 'b';
                                    break;
                                case 3:
                                    output[outPos++] = 'C';
                                    output[outPos++] = 'r';
                                    break;
                                case 4:
                                    output[outPos++] = 'R';
                                    break;
                                case 5:
                                    output[outPos++] = 'G';
                                    break;
                                case 6:
                                    output[outPos++] = 'B';
                                    break;
                            }
                        }
                        status = ERR_OK;
                        break;


                    case 0xA300:    // FileSource
                        output = malloc(30);

                        if (output == NULL) {
                            status = ERR_MALLOC_UNDEFINED_TRANS; 
                            break;
                        }

                        if (count > 4) {
                            snprintf(&output[0], 30, "%s", "Unknown");
                            break;
                        } 

                        switch(value[0]) {
                            case 1:
                                snprintf(&output[0], 30, "%s", "Film Scanner");
                                break; 
                            case 2:
                                snprintf(&output[0], 30, "%s", "Reflection Print Scanner");
                                break;
                            case 3:
                                snprintf(&output[0], 30, "%s", "Digital Camera");
                                break;
                            default: 
                                snprintf(&output[0], 30, "%s", "Unknown");
                                break;
                        }
                    status = ERR_OK;
                    break;

                    case 0xA301: // SceneType

                        output = malloc(25);

                        if(output == NULL) {
                            status = ERR_MALLOC_UNDEFINED_TRANS;
                            break;
                        }

                        if (count > 4) {
                            snprintf(&output[0], 25, "%s", "Unknown");
                            break;
                        }

                        if(value[0] == 1) {
                            snprintf(&output[0], 25, "%s", "Directrly Photographed");

                        } else {
                            snprintf(&output[0], 25, "%s", "Unknown");
                        }

                    status = ERR_OK;
                    break;
                }
                break;

            // **** SLONG **** //
            case 0x0009: //slong

                output = malloc(15);
                
                if(output == NULL) {
                    status = ERR_MALLOC_LONG_TRANS;
                    break;
                }

                // if error in the count
                if (count > 1) {
                    status = ERR_LONG_COUNT;
                    break;
                }

                

                // Adding the item
                snprintf(&output[0], sizeof(output), "%d", (int32_t)((
                    value[0] << 24 | 
                    value[1] << 16 |
                    value[2] << 8  |
                    value[3])
                ));
                status = ERR_OK;
            
                break;

            // **** SRATIONAL **** //
            case 0x000A: //srational
                output = malloc(30);
                if(output == NULL) {
                    status = ERR_MALLOC_RATIONAL_TRANS;
                }

                status = translate_srational(count, value, offset, buffer, output);
                break;
            default: return ERR_INVALID_TAG;
        }


     // Increment iterate past the just-read tagF


        if(status == ERR_OK) { 
            const char *tagName = get_exif_tag_name(tag);

            int valLen = strlen(output);
            int tagLen = strlen(tagName);

            if( strcmp(tagName, "unknown") && pos + valLen + tagLen < outputCap && tag != 0x8769 ) {

                outputBuffer[pos++] = '"';
                for(int it = 0; it < tagLen; it++) {
                    outputBuffer[pos++] = tagName[it];
                }
                outputBuffer[pos++] = '"';
                outputBuffer[pos++] = ':';
                outputBuffer[pos++] = '"';
                for(int it = 0; it < valLen; it++) {
                    outputBuffer[pos++] = output[it];
                }
                outputBuffer[pos++] = '"';
                outputBuffer[pos++] = ',';
            }

            free(output);
        }
    




    }


    // overwrite last comma
    outputBuffer[--pos] = '}';
    outputBuffer[++pos] = '\0'; 

    // printf("%s", outputBuffer);

    return ERR_OK;
    
}

static ErrorCode translate_byte(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output) {

    // fetches the value from the tag
    const uint32_t value = ((val_or_off[0]) << 24 |
                            (val_or_off[1]) << 16 |
                            (val_or_off[2]) << 8  |
                            (val_or_off[3]));
    
    // if the count is under 4 bytes look at tag
    if (count <= 4) {

        if (output == NULL) {
            return ERR_MALLOC_BYTE_TRANS;
        }

        snprintf(output, 11, "0x%08X", value);

        // Success
        return ERR_OK;

    // if the count is too large look at offset
    } else {
        size_t maxlen = (count + 3);
        char *output = malloc(maxlen);
        int pos = 0;
        
        // if output did not allocate properly
        if (output == NULL) {
            return ERR_MALLOC_BYTE_TRANS;
        }

        // itterate over every byte and append it to the output buffer
        for (int j = offset + value; j < count + offset + value; j++) {
            int written = snprintf(&output[pos], maxlen, "0x%02X ", buffer[j]);
            if (written < 0 || written >= (int)(sizeof(output) - pos)) {
                break;
            }
            pos += written;
        }

        // Success
        return ERR_OK;
    }

}

static ErrorCode translate_ascii(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output) {
    


    // iterator for tracking appending to string
    size_t pos = 0;




    // less than 4 bytes
    if (count <= 4) {

        for (int j = 0; j < count; j++) {
            char c = (char)val_or_off[j];
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

        // fetches the value from the tag
        const uint32_t value = ((val_or_off[0]) << 24 |
                                (val_or_off[1]) << 16 |
                                (val_or_off[2]) << 8  |
                                (val_or_off[3]));

        for (int j = offset + value; j < offset + value + count; j++) {

            char c = (char)buffer[j];
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



static ErrorCode translate_rational(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output) {

    // fetches the value from the tag
    const uint32_t value = ((val_or_off[0]) << 24 |
                            (val_or_off[1]) << 16 |
                            (val_or_off[2]) << 8  |
                            (val_or_off[3]));


    // Check that the count is only one rational
    if (count != 1) {
       return ERR_RATIONAL_COUNT;
    }

    size_t dataLoc = offset + value;

    // Read the values from the resgister
    const uint32_t numerator = (buffer[dataLoc++] << 24 |
                                buffer[dataLoc++] << 16 |
                                buffer[dataLoc++] << 8  |
                                buffer[dataLoc++]);

    const uint32_t denominator = (buffer[dataLoc++] << 24 |
                                  buffer[dataLoc++] << 16 |
                                  buffer[dataLoc++] << 8  |
                                  buffer[dataLoc++]);

    // convert the data into a string
    snprintf(&output[0], sizeof(output), "%d/%d", numerator, denominator);


    return ERR_OK;
}

static ErrorCode translate_srational(const uint32_t count, const uint8_t *val_or_off, size_t offset, const uint8_t *buffer, char *output) {

    // fetches the value from the tag
    const uint32_t value = ((val_or_off[0]) << 24 |
                            (val_or_off[1]) << 16 |
                            (val_or_off[2]) << 8  |
                            (val_or_off[3]));


    // Check that the count is only one rational
    if (count != 1) {
       return ERR_RATIONAL_COUNT;
    }

    size_t dataLoc = offset + value;

    // Read the values from the resgister
    const int32_t numerator = (buffer[dataLoc++] << 24 |
                                buffer[dataLoc++] << 16 |
                                buffer[dataLoc++] << 8  |
                                buffer[dataLoc++]);

    const int32_t denominator = (buffer[dataLoc++] << 24 |
                                  buffer[dataLoc++] << 16 |
                                  buffer[dataLoc++] << 8  |
                                  buffer[dataLoc++]);

    // convert the data into a string
    snprintf(&output[0], sizeof(output), "%d/%d", numerator, denominator);


    return ERR_OK;
}


