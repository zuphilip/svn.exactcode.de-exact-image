
struct deskew_rect {
  double x, y, width, height, angle;
};

deskew_rect deskewParameters (Image& image, int background_lines);
bool deskew (Image& image, int background_lines);
