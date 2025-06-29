#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *filename = "tests/example.jpeg";

    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);


    // Allocate buffer
    unsigned char *buffer = malloc(filesize);
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

        // Example: print first 8 byte as hex
        for(int i = 0; i < 8 && i < filesize; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("/n");

        // Cleanup
        free(buffer);
        fclose(file);
        return 0;
    }
}