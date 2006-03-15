
unsigned char*
read_JPEG_file (const char* filename, int* w, int* h, int* bps, int* spp,
                int* xres, int* yres);

void
write_JPEG_file (const char* file, unsigned char* data, int w, int h, int bps, int spp,
                 int xres, int yres);
