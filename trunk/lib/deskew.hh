
struct deskew_rect {
  double x, y, width, height, angle;
  double x_back, y_back;
};

deskew_rect deskewParameters (Image& image, int background_lines);
bool deskew (Image& image, int background_lines);
