
class Viewer {
public:
  
  Viewer(const std::vector<std::string>& _images)
    : images(_images), evas_data(0), zoom(100), evas_image(0) {
    it = images.begin();
    image = new Image;
  }
  ~Viewer() {
    delete (image); image = 0;
  }
  
  bool Load ();
  bool Next ();
  bool Previous ();

  int Run (bool opengl = false);
  
protected:
  
  void ImageToEvas ();
  
  void Zoom (double factor);
  void Move (int _x, int _y);
  
  int Window2ImageX (int x);
  int Window2ImageY (int y);
  
  void UpdateOSD (const std::string& str1, const std::string& str2);
  void AlphaOSD (int a);
  void TickOSD ();

  void SetOSDZoom ();
  
private:
  const std::vector<std::string>& images;
  std::vector<std::string>::const_iterator it;
  
  // Image
  Image* image;
  uint8_t* evas_data;
  
  // on screen display
  EvasRectangle* evas_osd_rect;
  EvasText* evas_osd_text1;
  EvasText* evas_osd_text2;
  Utility::Timer osd_timer;
  
  int zoom;
  int channel;
  
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
