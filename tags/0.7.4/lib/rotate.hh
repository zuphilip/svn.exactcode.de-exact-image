#include "Image.hh"

void flipX (Image& image);
void flipY (Image& image);

void shear (Image& image, double xangle, double yangle);
void rotate (Image& image, double angle, const Image::iterator& background);

void exif_rotate(Image& image, unsigned exif_orientation);

Image* copy_crop_rotate (Image& image, int x_start, int y_start,
			 unsigned int w, unsigned int h,
			 double angle, const Image::iterator& background);
