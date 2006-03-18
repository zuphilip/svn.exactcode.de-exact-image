
void normalize (Image& image, unsigned char low = 0, unsigned char high = 0);

void colorspace_rgb_to_gray (Image& image);
void colorspace_gray_to_bilevel (Image& image, unsigned char threshold = 127);

void colorspace_gray_to_rgb (Image& image);
void colorspace_bilevel_to_gray (Image& image);

void colorspace_16_to_8 (Image& image);

void colorspace_de_palette (Image& image, int table_entries,
			    uint16_t* rmap, uint16_t* gmap, uint16_t* bmap);
