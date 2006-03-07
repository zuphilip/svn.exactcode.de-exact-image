
class Viewer {
public:
  
  Viewer() : zoom(1.0) {}
  ~Viewer() {}
  
  int Run (Image* _image);
  
protected:
  
  void Zoom (double factor);
  
private:
  
  // Image
  Image* image;
  double zoom;
  
  // X11 stuff
  Display* dpy;
  int scr;
  Visual* visual;
  Window  win;
  int depth;
  
  // evas
  EvasImage* evas_image;
};
