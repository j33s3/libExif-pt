#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "exif_parser.h"

int main() {
  const char *filename = "tests/example.jpeg";


  // Reads the file copntents into file
  FILE *file = fopen(filename, "rb");
  // If the file connot be opened return error message
  if (!file) {
      printf("Failed to open file tests/example.jpeg\n");
      return 1;
  }
  printf("File opened successfully!\n");


  // Moves the file pointer to the end of the  file
  fseek(file, 0, SEEK_END);

  printf("SEEK END\n");

  // Get how many bytes from start to end
  size_t filesize = ftell(file);

  // Resets the file pointer back to the beginning of the file
  rewind(file);
  printf("REWIND FILE\n");

  // Allocate buffer
  uint8_t *buffer = malloc(filesize);
  if (!buffer) {
    perror("Failed to allocate buffer");
    fclose(file);
    return 1;
  }
  printf("ALLOCATED BUFFER\n");

  // Read file into buffer
  size_t read = fread(buffer, 1, filesize, file);
  if (read != filesize) {
    perror("Failed to read file");
    free(buffer);
    fclose(file);
    return 1;
  }
  printf("READ FILE INTO BUFFER\n");



  printf("CREATING OUTPUT BUFFER\n");

  printf("File Length: %zu:\n", filesize);
  char *response = parse_jpeg(buffer, filesize);

  printf("Getting Response\n");

  fflush(stdout);


  printf("\n%s\n", response);
  fflush(stdout);


  // Cleanup
  free(buffer);
  fclose(file);
  return 0;
}