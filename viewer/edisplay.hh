
class Viewer {
public:
  
  Viewer() : zoom(100) {}
  ~Viewer() {}
  
  int Run (Image* _image);
  
protected:
  
  void Zoom (double factor);
  void Move (int _x, int _y);
  
private:
  int zoom;
  
  // Image
  Image* image;
  
  // X11 stuff
  Display* dpy;
  int scr;
  Visual* visual;
  Window  win;
  int depth;
  
  // evas
  EvasCanvas* evas;
  EvasImage* evas_image;
};
