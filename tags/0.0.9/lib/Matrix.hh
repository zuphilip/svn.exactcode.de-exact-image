
// any matrix and devisior
// on my AMD Turion speed is: double > int > float
// and in 32bit mode: double > float > int ?
typedef double matrix_type;

void convolution_matrix (Image& image, matrix_type* matrix, int xw, int yw,
			 matrix_type divisor)
{
  unsigned char *data2 = (unsigned char *) malloc (image.w * image.h);
  
  int xr = xw / 2;
  int yr = yw / 2;
  
#if 0
  // experimental tiling code that is not faster :-(
  const int tiles = 16;
  for (int my = 0; my < h; my += tiles)
    for (int mx = 0; mx < w; mx += tiles)
      for (int y = my; y < my + tiles && y < h; ++y)
	for (int x = mx; x < mx + tiles && x < w; ++x)
#else
  for (int y = 0; y < image.h; ++y)
    for (int x = 0; x < image.w; ++x)
#endif
      {
	unsigned char * const dst_ptr = &data2[x + y * image.w];
	unsigned char * const src_ptr = &image.data[x + y * image.w];
	
	const unsigned char val = *src_ptr;
	
	// for now, copy border pixels
	if (y < yr || y > image.h - yr ||
	    x < xr || x > image.w - xr)
	  *dst_ptr = val;
	else {
	  matrix_type sum = 0;
	  unsigned char* data_row =
	    &image.data[ (y - yr) * image.w - xr + x];
	  matrix_type* matrix_row = matrix;
	  // in former times this was more readable and got overoptimized
	  // for speed ,-)
	  for (int y2 = 0; y2 < yw; ++y2, data_row += image.w - xw)
	    for (int x2 = 0; x2 < xw; ++x2)
	      sum += *data_row++ * *matrix_row++;
	  
	  sum /= divisor;
	  unsigned char z = (unsigned char)
	    (sum > 255 ? 255 : sum < 0 ? 0 : sum);
	  *dst_ptr = z;
	}
      }
  
  free (image.data);
  image.data = data2;
}
