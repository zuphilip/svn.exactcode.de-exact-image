

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* read_bmp (const char* file, int* w, int* h, int* bps, int* spp,
			 int* xres, int* yres, unsigned char** color_table);

#ifdef __cplusplus
}
#endif
