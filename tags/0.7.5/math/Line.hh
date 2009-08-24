
#include <cmath>
#include <functional>

class Line {
public:
  typedef std::pair <double, double> point;
    
  Line (point _p1, point _p2)
    : p1 (_p1), p2 (_p2) {};
    
  Line (double x1, double y1, double x2, double y2)
    : p1 (x1, y1), p2 (x2, y2) {};
    
  bool intersection (const Line& other, point& p)
  {
    const double det =
      (other.p2.second - other.p1.second) * (p2.first - p1.first) -
      (other.p2.first - other.p1.first) * (p2.second - p1.second);
      
    if (det == 0)  // parallel
      return false;
      
    double Ua =
      (other.p2.first - other.p1.first) * (p1.second - other.p1.second) -
      (other.p2.second - other.p1.second) * (p1.first - other.p1.first);
      
    Ua /= det;
      
    p.first = p1.first + Ua * (p2.first - p1.first);
    p.second = p1.second + Ua * (p2.second - p1.second);
    return true;
  }
    
  point& begin () { return p1; };
  point& end () { return p2; };
    
  point mid () {
    return point (p1.first + (p2.first - p1.first) / 2,
		  p1.second + (p2.second - p1.second) / 2);
  };
    
  double length () {
    const double xrel = std::abs (p1.first - p2.first);
    const double yrel = std::abs (p1.second - p2.second);
    return sqrt (xrel*xrel + yrel*yrel);
  }
    
  double angle () {
    double a;
    if (p2.first >= p1.first) {
      a = atan ((p2.second - p1.second) / (p2.first - p1.first));
      if (a < 0)
	a += 2*M_PI;
    }
    else
      a = atan ((p1.second - p2.second) / (p1.first - p2.first)) + M_PI;
    return a;
  }
    
#ifdef DEBUG
  // just quick
  void draw (Path& path, Image& image, const Image::iterator& color) {
    double r, g, b; color.getRGB (r, g, b);
    path.setFillColor (r, g, b);
    path.moveTo (p1.first, p1.second);
    path.addLineTo (p2.first, p2.second);
    path.draw (image);
    path.clear ();
  };
#endif
    
private:
  point p1, p2;
};
