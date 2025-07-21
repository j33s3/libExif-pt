/*
 * @file            src/exif_parser.c
 * @description
 * @author          Jesse Peterson
 * @createTime      2025-06-27 22:51:55
 * @lastModified    2025-07-18 09:33:03
 */

#include "exif_parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ** COMPILE WITH VERBOSE ** //
#ifdef VERBOSE
#define VPRINT(...) printf(__VA_ARGS__)
#else
#define VPRINT(...)                                                            \
  do {                                                                         \
  } while (0)
#endif

// **** ERROR HANDLING **** //

static char *get_error_string(ErrorCode code) {
switch (code) {
    case ERR_OK:
        return "No Error";
    case ERR_EXIF_MISSING:
        return "Missing EXIF data";
    case ERR_TIFF_OVERFLOW: 
        return "TIFF extends past image length";
    case ERR_EXIF_OVERFLOW:
        return "EXIF extends past image length";
    case ERR_ENDIAN_MISSING:
        return "Missing Endianness";
    case ERR_TIFF_MISSING:
        return "Missing TIFF header";
    case ERR_INVALID_TAG:
        return "Tag is unkown or invalid";
    case ERR_MALLOC:
        return "Error during malloc";
    case ERR_SHORT_COUNT:
        return "The count of a short item exceeds 1";
    case ERR_LONG_COUNT:
        return "The count of a long item exceeds 1";
    case ERR_RATIONAL_COUNT:
        return "The count of rational items exceeds 1";
    case ERR_UNKNOWN_UNDEFINED:
        return "The tag of type undefined is unknown";
    case ERR_UNKNOWN:
        return "Unkown Error";
    default:
        return "Unkown";
    }
}

// **** EXIF TAGS **** //

static const char *get_exif_tag_name(uint16_t tag) {

  size_t count = sizeof(exif_tags) / sizeof(exif_tags[0]);
  for (size_t i = 0; i < count; i++) {
    if (exif_tags[i].tag == tag) {
      return exif_tags[i].name;
    }
  }

  return "unknown";
}

// **** PARSER **** //
char *parse_jpeg(const uint8_t *buffer, size_t length) {

    size_t i = 2;                                               // SKIP SOI (0xFF, 0xD8)
    uint16_t seg_length = 0;                                    // Track how long the Exif chunk is
    char *output = malloc(2);                                    // Allocate memory to storing the output
    output[0] = '{';
    output[1] = '\0';

    if (*output == NULL) {                                       // If Malloc fails
        return get_error_string(ERR_MALLOC);
    }

    while (i + 4 < length) {                                    // i + 4 to SOI EOI and JFIF
        
        if (((buffer[i] << 8) | buffer[i + 1]) == 0xFFE0) {     // If JFIF is present look for the offset
            i += (((buffer[i + 2] << 8) | buffer[i + 3]) + 1);
        }

        if (((buffer[i] << 8) | buffer[i + 1]) == 0xFFE1) {     // EXIF MARKER
            seg_length = buffer[i + 2] << 8 | buffer[i + 3];    // Set the segment length to the length outlined here

            if(i + seg_length > length - 2 ) {                  // If the Tiff segment extends past image buffer
                return get_error_string(ERR_TIFF_OVERFLOW);
            }

            if ((i + 9 < seg_length) && (                           // Checks that we do not extend past seg_length
                buffer[i + 4] == 'E' &&
                buffer[i + 5] == 'x' && 
                buffer[i + 6] == 'i' &&
                buffer[i + 7] == 'f' &&
                buffer[i + 8] == 0x00 && 
                buffer[i + 9] == 0x00)) {
                    i += 10;                                         // Accounts for our checks
                    u8_crawler(buffer, seg_length, i, &output);
                    return output;
            }
        }
        i++;
    }
    return NULL;
}

static ErrorCode u8_crawler(const uint8_t *buffer, uint16_t seg_length, size_t offset, char **output) {

    uint16_t tiff_tags = 0;                                     // Length of tiff tags
    uint16_t exif_tags = 0;                                     // Length of exif tags
    bool big_endian = false;                                    // Tracks the endianess
    size_t itt = offset;                                        // Itterator


    VPRINT("| Endian bytes: 0x%04X ", ((buffer[itt] << 8)| buffer[itt + 1]));

    switch((buffer[itt] << 8) | buffer[itt + 1]) {                  // Tracks the endianess
        case (0x4D4D):
            big_endian = true;
            break;
        case (0x4949):
            big_endian = false;
            break;
        default:
            return ERR_ENDIAN_MISSING;
    }
    itt += 2;

    VPRINT("| big_endian: %d |\n", big_endian);                     // Verbose logging

    if(                                                              // IF TIFF magic number is missing
        (big_endian && ((buffer[itt] << 8) | buffer[itt + 1]) != 0x002A) ||
        (!big_endian &&((buffer[itt + 1] << 8) | buffer[itt]) != 0x002A)) {  
        return ERR_TIFF_MISSING;
    }
    itt += 6;                                                   // jump number and offset to first IFD

    if (big_endian) {
        tiff_tags = ((buffer[itt] << 8) | buffer[itt + 1]);             // Set the tiff_tags
    } else {
        tiff_tags = ((buffer[itt + 1] << 8) | buffer[itt]);             // Set the tiff_tags
    }
    itt += 2;

    VPRINT("| # of tiff_tags: %d |\n", tiff_tags);

    for(int i = 0; i < tiff_tags + exif_tags; i++) {            // Iterates through Tiff then exif tags

        // ** TAG ** //
        uint16_t tag = 0;
        if (big_endian) {
            tag = (buffer[itt] << 8) | buffer[itt + 1];   // Gets the tag
        } else {
            tag = (buffer[itt + 1] << 8) | buffer[itt];   // Gets the tag
        }
        itt += 2;

        VPRINT("| Tag: %s ", get_exif_tag_name(tag));

        // ** TYPE ** //
        uint16_t type = 0;
        if (big_endian) {
            type = (buffer[itt] << 8) | buffer[itt + 1];   // Gets the type
        } else {
            type = (buffer[itt + 1] << 8) | buffer[itt];   // Gets the type
        }
        itt += 2;

        VPRINT("| Type: 0x%04X ", type);

        // ** COUNT ** //
        uint32_t count = 0;
        if (big_endian) {
            count = (                                // Gets the count
                (buffer[itt] << 24) | 
                (buffer[itt + 1] << 16) |
                (buffer[itt + 2] << 8) |
                (buffer[itt + 3])
            ); 
        } else {
            count = (                                // Gets the count
                (buffer[itt + 3] << 24) | 
                (buffer[itt + 2] << 16) |
                (buffer[itt + 1] << 8) |
                (buffer[itt + 0])
            ); 
        }
        itt += 4;

        VPRINT("| Count: %d ", type);

        // ** VALUE ** //
        uint8_t value[20];                                // Gets the value in UINT_8
        for (int j = 0; j < 4; j++) {
            value[j] = buffer[itt++];
        }


        ErrorCode status;                                       // Use this for tracking error codes
        char *response = malloc(1);                        // Use this char string to track responses
        response[0] = '\0';

        // **** Inline-Data parsing**** //
        switch (type) {
            // ** BYTE ** //
            case 0x0001: {
                status = translate_byte(buffer, count, value, offset, &response, big_endian);
                break;
            }
            // ** ASCII ** //
            case 0x0002: {
                status = translate_ascii(buffer, count, value, offset, &response, big_endian);
                break;
            }
            // ** SHORT ** //
            case 0x0003: {
                status = translate_short(buffer, count, value, offset, &response, big_endian, tag);
                break;
            }
            // ** LONG ** //
            case 0x0004: {

                if (exif_tags == 0 && tag == 0x8769) {          // If the tag is ExifOffset then jump the iterator to our exif data
                    if(big_endian) {
                        itt = offset + ((value[0] << 24) |
                                        (value[1] << 16) |
                                        (value[2] << 8) |
                                        (value[3]));
                        exif_tags = ((buffer[itt] << 8) | buffer[itt + 1]);

                    } else {
                        itt = offset + ((value[3] << 24) |
                                        (value[2] << 16) |
                                        (value[1] << 8) |
                                        (value[0]));
                        exif_tags = ((buffer[itt + 1] << 8) | buffer[itt]);
                    }
                    itt += 2;
                    
                }

                status = translate_long(buffer, count, value, offset, &response, big_endian);
                break;
            }
            // ** RATIONAL ** //
            case 0x0005: {
                status = translate_rational(buffer, count, value, offset, &response, big_endian);
                break;
            }
            // ** UNDEFINED ** //
            case 0x0007: {
                status = translate_undefined(buffer, count, value, offset, &response, big_endian, tag);
                break;
            }
            // ** SLONG ** //
            case 0x0009: {
                status = translate_slong(buffer, count, value, offset, &response, big_endian);
                break;
            } 
            // ** SRATIONAL ** //
            case 0x000A: {
                status = translate_srational(buffer, count, value, offset, &response, big_endian);
                break;
            }

        }
                                    //TEMP DISABLE UNDEFINED
        if(status == ERR_OK && type != 0x0007) {                                          // If the response is valid
            const char *tagName = get_exif_tag_name(tag);


            if(tag != 0x8769 && (strcmp(tagName, "unknown") != 0)) {    // If the tag is not exifOffset and not unkown

                size_t valLen = strlen(response);                       // Get the length of response string
                size_t tagLen = strlen(tagName);                        // Get the length of tagName
                char str[1024];                                             // Create a new string to format the data


                if(tag != 0xA001 && (type == 0x03 || type == 0x04)) {
                    snprintf(str, 1024, "\"%s\":%s,", tagName, response);
                } else {
                    snprintf(str, 1024, "\"%s\":\"%s\",", tagName, response);
                }
            
                size_t new_len = (strlen(*output) + strlen(str) + 1);

                char *tmp = realloc(*output, new_len);
                if (tmp == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    return ERR_UNKNOWN; // or handle the error as needed
                }
                *output = tmp;

                strcat(*output, str);
                VPRINT("| %s |\n", str);
            }
        }
        free(response);
    }

    if (strlen(*output) >= 1) {
        (*output)[strlen(*output) - 1] = '}';
    };

    return ERR_OK;
    
}

static ErrorCode translate_byte(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {
    
    if (count <= 4) {
        
        for(uint8_t i = 0; i < count; i++) {
            char hexString[7];
                                                            
            if (i == count - 1) {                                       // Just format the bytes to string and return
                snprintf(hexString, 7, "0x%02X", val_or_off[i]);
            } else {
                snprintf(hexString, 7, "0x%02X, ", val_or_off[i]);
            }

            size_t new_len = (strlen(*response) + strlen(hexString) + 1);

            char *temp = realloc(*response, new_len);
            if (!temp) {
                perror("realloc failed");
                return ERR_UNKNOWN;
            }
            *response = temp;
            
            strcat(*response, hexString);
        }

        VPRINT("| BYTE: %s | ", *response);
        return ERR_OK;
    } else {
        return ERR_UNKNOWN;                                             // Can be changes later, dont believe there are any tags we use that are bytes                                                                      
    }
}

static ErrorCode translate_ascii(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {
    char str[count + 1];                                               // For storing the string for concatenation
    size_t pos = 0;                                                     // Tracks our current location on the str

    if (count <= 4) {                                                   // If value is inside of the inline-value

        for(size_t i = 0; i < count; i++) {                             // Iterate over all of items BUT break when meeting a null terminator '\0
            
            if(val_or_off[i] == '\0') break;

            char c = (char)val_or_off[i];                               // Cast the current byte to a character
            if(isprint(c)) {                                            // Check if it is a character
                str[pos++] = c;                                         // Add it to the str
            } else {
                str[pos++] = '.';                                       // Otherwise add a '.'
            }
        }
        str[pos] = '\0';                                             // Cap the item with a null terminator

    } else {

        uint32_t value = 0;

        if (big_endian) {
            value = (
                (val_or_off[0] << 24) |
                (val_or_off[1] << 16) |
                (val_or_off[2] << 8) |
                (val_or_off[3])
            );
        } else {
            value = (
                (val_or_off[3] << 24) |
                (val_or_off[2] << 16) |
                (val_or_off[1] << 8) |
                (val_or_off[0])
            );
        }

        for (size_t i = offset + value; i < ( offset + value + count); i++) {
                                                                        // Iterate over all of items BUT break when meeting a null terminator '\0'
            
            if (buffer[i] == '\0') break;             
            
            char c = (char)buffer[i];                               // Try to cast the byte to a character
            if(isprint(c)) {                                            // If valid add to string else '.'
                str[pos++] = c;
            } else {
                str[pos++] = '.';
            }
        }
        str[pos] = '\0';
    }

    size_t new_len = ((strlen(*response) + strlen(str)) + 1);                // Calculates the new length of the string
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                          // Concatenate new data with response pointer
    
    VPRINT("| ASCII: %s | ", *response);
    return ERR_OK;
}


static ErrorCode translate_short(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian, const uint16_t tag) {
    if (count > 1) return ERR_SHORT_COUNT;                          // If the count of the short is more than one return error

    char str[16];

    uint16_t value = 0;         
    if(big_endian) {                                                // Get the value either big or little endian
        value = ((val_or_off[0] << 8) | val_or_off[1]);
    } else {
        value = ((val_or_off[1] << 8) | val_or_off[0]);
    }

    if (tag == 0xA001) {                                            // COLOR SPACE
        switch ((val_or_off[0] << 8) | val_or_off[1]) {
        case 0x1:
            snprintf(str, 5, "%s", "sRGB");
            break;
        case 0x2:
            snprintf(str, 10, "%s", "Adobe RBG");
            break;
        case 0xFFFD:
            snprintf(str, 15, "%s", "Wide Gamut RGB");
            break;
        case 0xFFFE:
            snprintf(str, 12, "%s", "ICC Profile");
            break;
        case 0xFFFF:
            snprintf(str, 13, "%s", "Uncalibrated");
            break;
        default:
            snprintf(str, 8, "%s", "Unknown");
            break;
        }
    } else {                                                        // Otherwise append the number
        snprintf(str, 6, "%d", (uint16_t)((val_or_off[0] << 8) | val_or_off[1]));
    }

    size_t new_len = ((strlen(*response) + strlen(str)) + 1);    // Calculate the new length or response
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                  // Concatenate strings

    VPRINT("| SHORT: %s | ", *response);


    return ERR_OK;
}
static ErrorCode translate_long(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {
    if (count > 1) return ERR_LONG_COUNT;                           // If count is more than 1 long

    uint32_t value = 0;                                             // Tracking the value
    if (big_endian) {
        value = (
            (val_or_off[0] << 24) |
            (val_or_off[1] << 16) |
            (val_or_off[2] << 8) |
            (val_or_off[3])
        );
    } else {
        value = (
            (val_or_off[3] << 24) |
            (val_or_off[2] << 16) |
            (val_or_off[1] << 8) |
            (val_or_off[0])
        );
    }

    char str[12];
    snprintf(str, 12, "%u", value);                             // Moves the value into a string

    size_t new_len = (strlen(*response) + strlen(str) + 1);            // Calculate the string length
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                  // Concatenate strings

    VPRINT("| LONG: %s | ", *response);

    return ERR_OK;

}
static ErrorCode translate_rational(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {
    if( count > 1) return ERR_RATIONAL_COUNT;

    uint32_t value = 0;                                             // Stores the offset value
    uint32_t numerator = 0;                                         // Stores the numerator
    uint32_t denominator = 0;                                       // Stores the denominator

    if (big_endian) {                                               // Gets the value depending on endianness
        value = (
            (val_or_off[0] << 24) |
            (val_or_off[1] << 16) |
            (val_or_off[2] << 8) |
            (val_or_off[3])
        );
    } else {
        value = (
            (val_or_off[3] << 24) |
            (val_or_off[2] << 16) |
            (val_or_off[1] << 8) |
            (val_or_off[0])
        );
    }

    uint8_t *locale = &buffer[offset + value];                      // Creates a pointer to the rational location


    if (big_endian) {                                               // Gets the numerator and denominator
        numerator = (
            (locale[0] << 24) |
            (locale[1] << 16) |
            (locale[2] << 8) |
            (locale[3]));

        denominator = (
            (locale[4] << 24) |
            (locale[5] << 16) |
            (locale[6] << 8) |
            (locale[7]));
    } else {
        numerator = (
            (locale[3] << 24) |
            (locale[2] << 16) |
            (locale[1] << 8) |
            (locale[0]));

        denominator = (
            (locale[7] << 24) |
            (locale[6] << 16) |
            (locale[5] << 8) |
            (locale[4]));
    }

    char str[25];                                                  // String to format this item
    snprintf(str, 25, "%u/%u", numerator, denominator);

    size_t new_len = (strlen(*response) + strlen(str) + 1);            // Calculate the new length of response
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                  // concatenate strings

    VPRINT("| RATIONAL: %s | ", *response);

    return ERR_OK;
}

static ErrorCode translate_undefined(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian, const uint16_t tag) {

    switch(tag) {
        case 0x9000:                                                // ** ExifVersion
        case 0xA000: {                                               // ** FlashpixVersion

            if (count > 4) return ERR_UNKNOWN_UNDEFINED;             // IF the length is more than 4 bytes then return error

            char str[10];                                           // Track string and the size of it
            size_t pos = 0;
            
            for (int i = 0; i < 4; i++) {
                char c = (char)val_or_off[i];                       // Iterate through all bytes
                str[pos++] = c;
                if (pos == 2) {                                     // When reaching second byte insert '.' in the middle of the string
                    str[pos++] = '.';
                }
            }

            size_t new_len = (strlen(*response) + pos + 1);                // Calculate the new length of response
            char *temp = realloc(*response, new_len);
            if (!temp) {
                perror("realloc failed");
                free(*response);
                response = NULL;
                return ERR_UNKNOWN;
            }
            *response = temp;

            strcat(*response, str);                                  // Concatenate strings

            VPRINT("| UNDEFINED: %s | ", *response);

            return ERR_OK;
        }
        case 0x9101: {                                               // ** ComponentConfiguration

            if (count > 4) return ERR_UNKNOWN_UNDEFINED;              // If the length is more than 4 bytes then return error

            char str[10];                                          // Track string and the size of it    
            size_t pos = 0;

            for (int it = 0; it < 4; it++) {                        // Iterate over ever byte and decode values
                switch (val_or_off[it]) {
                case 0:
                    str[pos++] = '-';
                    break;
                case 1:
                    str[pos++] = 'Y';
                    break;
                case 2:
                    str[pos++] = 'C';
                    str[pos++] = 'b';
                    break;
                case 3:
                    str[pos++] = 'C';
                    str[pos++] = 'r';
                    break;
                case 4:
                    str[pos++] = 'R';
                    break;
                case 5:
                    str[pos++] = 'G';
                    break;
                case 6:
                    str[pos++] = 'B';
                    break;
                }
            }

            size_t new_len = (strlen(*response) + pos + 1);              // Calculate the new length of response
            char *temp = realloc(*response, new_len);
            if (!temp) {
                perror("realloc failed");
                free(*response);
                response = NULL;
                return ERR_UNKNOWN;
            }
            *response = temp;

            strcat(*response, str);                      // Concatenate strings

            VPRINT("| UNDEFINED: %s | ", *response);

            return ERR_OK;
        }
        case 0xA300: {                                           // ** FileSource
            
            if (count > 4) return ERR_UNKNOWN_UNDEFINED;        // If length is more than 4 bytes then return error

            char str[30];                                       // New string of max 30 characters

            switch (val_or_off[0]) {                                 // Set the str to decoded string
            case 1:
                snprintf(str, 30, "%s", "Film Scanner");
                break;
            case 2:
                snprintf(str, 30, "%s", "Reflection Print Scanner");
                break;
            case 3:
                snprintf(str, 30, "%s", "Digital Camera");
                break;
            default:
                snprintf(str, 30, "%s", "Unknown");
            break;
            }
            
            size_t new_len = (strlen(*response) + strlen(str) + 1);// Calculate the new length of response
            char *temp = realloc(*response, new_len);
            if (!temp) {
                perror("realloc failed");
                free(*response);
                *response = NULL;
                return ERR_UNKNOWN;
            }
            *response = temp;

            strcat(*response, str);                      // Concatenate strings

            VPRINT("| UNDEFINED: %s | ", *response);

            return ERR_OK;                                     
        }
        case 0xA301: {                                           // ** SceneType
            if (count > 4) return ERR_UNKNOWN_UNDEFINED;         // If the length is more than 4 byte then return error

            char str[25];
            
            if(val_or_off[0] == 1) {                            // 1 if Directly Photographed otherwise something else
                snprintf(str, 25, "%s", "Directly Photographed");
            } else {
                snprintf(str, 25, "%s", "Unknown");
            }

            size_t new_len = (strlen(*response) + strlen(str) + 1);// Calculate the new length of response
            char *temp = realloc(*response, new_len);
            if (!temp) {
                perror("realloc failed");
                free(*response);
                response = NULL;
                return ERR_UNKNOWN;
            }
            *response = temp;

            strcat(*response, str);                       // Concatenate strings 

            VPRINT("| UNDEFINED: %s | ", *response);

            return ERR_OK;
        }            
        default:
            return ERR_UNKNOWN_UNDEFINED;
    }
    
}
static ErrorCode translate_slong(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {
    if (count > 1) return ERR_LONG_COUNT;                           // If count is more than 1 long

    int32_t value = 0;                                             // Tracking the value
    if (big_endian) {
        value = (
            (val_or_off[0] << 24) |
            (val_or_off[1] << 16) |
            (val_or_off[2] << 8) |
            (val_or_off[3])
        );
    } else {
        value = (
            (val_or_off[3] << 24) |
            (val_or_off[2] << 16) |
            (val_or_off[1] << 8) |
            (val_or_off[0])
        );
    }

    char str[12];
    snprintf(str, 12, "%d", value);                             // Moves the value into a string

    size_t new_len = (strlen(*response) + strlen(str) + 1);            // Calculate the string length
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                  // Concatenate strings

    VPRINT("| SLONG: %s | ", *response);

    return ERR_OK;

}
static ErrorCode translate_srational(const uint8_t *buffer, const uint32_t count, const uint8_t * val_or_off, size_t offset, char **response, const bool big_endian) {

    if( count > 1) return ERR_RATIONAL_COUNT;

    uint32_t value = 0;                                             // Stores the offset value
    int32_t numerator = 0;                                         // Stores the numerator
    int32_t denominator = 0;                                       // Stores the denominator

    if (big_endian) {                                               // Gets the value depending on endianness
        value = (
            (val_or_off[0] << 24) |
            (val_or_off[1] << 16) |
            (val_or_off[2] << 8) |
            (val_or_off[3])
        );
    } else {
        value = (
            (val_or_off[3] << 24) |
            (val_or_off[2] << 16) |
            (val_or_off[1] << 8) |
            (val_or_off[0])
        );
    }

    uint8_t *locale = &buffer[offset + value];                      // Creates a pointer to the rational location


    if (big_endian) {                                               // Gets the numerator and denominator
        numerator = (
            (locale[0] << 24) |
            (locale[1] << 16) |
            (locale[2] << 8) |
            (locale[3]));

        denominator = (
            (locale[4] << 24) |
            (locale[5] << 16) |
            (locale[6] << 8) |
            (locale[7]));
    } else {
        numerator = (
            (locale[3] << 24) |
            (locale[2] << 16) |
            (locale[1] << 8) |
            (locale[0]));

        denominator = (
            (locale[7] << 24) |
            (locale[6] << 16) |
            (locale[5] << 8) |
            (locale[4]));
    }

    char str[25];                                                  // String to format this item
    snprintf(str, 25, "%d/%d", numerator, denominator);

    size_t new_len = (strlen(str) + strlen(*response) + 1);            // Calculate the new length of response
    char *temp = realloc(*response, new_len);
    if (!temp) {
        perror("realloc failed");
        free(*response);
        response = NULL;
        return ERR_UNKNOWN;
    }
    *response = temp;

    strcat(*response, str);                                  // concatenate strings

    VPRINT("| SRATIONAL: %s | ", *response);

    return ERR_OK;

}
