#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "exif_parser.h"


int main() {
    const char *filename = "tests/example.jpeg";

    // Reads the file copntents into file
    FILE *file = fopen(filename, "rb");

    // If the file connot be opened return error message
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // Moves the file pointer to the end of the  file
    fseek(file, 0, SEEK_END);

    // Get how many bytes from start to end
    size_t filesize = ftell(file);

    // Resets the file pointer back to the beginning of the file
    rewind(file);


    // Allocate buffer
    uint8_t *buffer = malloc(filesize);
    if (!buffer) {
        perror("Failed to allocate buffer");
        fclose(file);
        return 1;
    }

    // Read file into buffer
    size_t read = fread(buffer, 1, filesize, file);
    if (read != filesize) {
        perror("Failed to read file");
        free(buffer);
        fclose(file);
        return 1;
    }

    parse_jpeg(buffer, filesize);

    printf("/n");

    // Cleanup
    free(buffer);
    fclose(file);
    return 0;
    }