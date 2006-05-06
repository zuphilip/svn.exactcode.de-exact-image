
class Viewer {
public:
  
  Viewer(const std::vector<std::string>& _images)
    : images(_images), zoom(100), evas_image(0) {
    it = images.begin();
    image = new Image;
  }
  ~Viewer() {
    delete (image); image = 0;
  }
  
  bool Load ();
  void Next ();
  void Previous ();

  int Run ();
  
protected:
  
  void Zoom (double factor);
  void Move (int _x, int _y);
  
private:
  const std::vector<std::string>& images;
  std::vector<std::string>::const_iterator it;
  
  // Image
  Image* image;
  
  int zoom;
  
  // X11 stuff
  Display* dpy;
  int scr;
  Visual* visual;
  Window  win;
  int depth;
  
  // evas
  EvasCanvas* evas;
  EvasImage* evas_image;
  EvasImage* evas_bgr_image;
};
