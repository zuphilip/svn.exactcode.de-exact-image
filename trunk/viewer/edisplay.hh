
class Viewer {
public:
  
  Viewer() {}
  ~Viewer() {}
  
  int Run (Image* _image);
  
protected:
  
  void Zoom (double factor);
  
private:
  
  // Image
  Image* image;
  
  // X11 stuff
  Display* dpy;
  int scr;
  Visual* visual;
  Window  win;
  int depth;
  
  // evas
  EvasImage* evas_image;
};
