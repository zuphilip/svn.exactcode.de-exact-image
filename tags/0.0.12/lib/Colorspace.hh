
void normalize (Image& image, unsigned char low = 0, unsigned char high = 0);

void colorspace_rgb8_to_gray8 (Image& image);

void colorspace_gray8_to_gray1 (Image& image, unsigned char threshold = 127);
void colorspace_gray8_to_gray2 (Image& image);
void colorspace_gray8_to_gray4 (Image& image);
void colorspace_gray8_to_rgb8 (Image& image);

void colorspace_grayX_to_rgb8 (Image& image);

void colorspace_gray1_to_gray2 (Image& image);
void colorspace_gray1_to_gray4 (Image& image);
void colorspace_gray1_to_gray8 (Image& image);

void colorspace_16_to_8 (Image& image);

void colorspace_de_palette (Image& image, int table_entries,
			    uint16_t* rmap, uint16_t* gmap, uint16_t* bmap);
