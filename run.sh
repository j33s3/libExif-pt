emcc -I include src/exif_parser.c tests/test_exif_parser.c \
  -DVERBOSE \
  -o output.html \
  --preload-file tests/example.jpeg@tests/example.jpeg \
  -g4 \
    --source-map-base http://localhost:3000/

