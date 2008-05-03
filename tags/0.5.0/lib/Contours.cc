#include "Contours.hh"

/*
 list of pixel traversals (clockwise order):
 0: on left side upwards
 1: on pixel top to the right
 2: on right side downwards
 3: on pixel bottom to the left
*/

// bitmask for pixel traversal - first bit is there to indicate foreground pixel
const unsigned int pixelborder[4]={2,4,8,16};

struct StartCheck {
  int dx;
  int dy;
};

const StartCheck startchecks[4]={
  {-1,0}, {0,-1}, {1,0}, {0,1}
};

struct Transition {
  int dx;
  int dy;
  unsigned int border;
};

const Transition transitions[4][3]={
  { {-1, -1, 3}, { 0, -1, 0}, {0, 0, 1} },
  { { 1, -1, 0}, { 1,  0, 1}, {0, 0, 2} },
  { { 1,  1, 1}, { 0,  1, 2}, {0, 0, 3} },
  { {-1,  1, 2}, {-1,  0, 3}, {0, 0, 0} }
};


inline bool Step(Contours::VisitMap& map, int& x, int& y, int& border)
{
  for (unsigned int i=0; i<3; i++) {
    const Transition& t=transitions[border][i];
    const int xx=x+t.dx;
    const int yy=y+t.dy;
    // do we have a foreground pixel ?
    if (xx >= 0 && xx < (signed int)map.w && yy >= 0 && yy < (signed int)map.h && map(xx,yy) > 0) {
      if ((map(xx,yy) & pixelborder[t.border]) == 0) { // go there
	x=xx;
	y=yy;
	border=t.border;
	map(x,y) |= pixelborder[border];
	return true;
      } else // already been there before
	return false;
    }
  }

  // note: this code line is never reached, because last transition is always {0,0,b}
  return false;
}

inline bool Start(Contours::VisitMap& map, int x, int y, int border)
{
  if ((map(x,y) & pixelborder[border]) == 0 ) {
    const StartCheck& c=startchecks[border];
    const int xx=x+c.dx;
    const int yy=y+c.dy;
    // do we have a background pixel ?
    if (xx < 0 || xx >= (signed int)map.w || yy < 0 || yy >= (signed int)map.h || ((map(xx,yy) & 1) == 0)) {
      map(x,y)|=pixelborder[border];
      return true;
    }
  }
  return false;
}

Contours::Contours(const FGMatrix& image)
{
  VisitMap map(image.w, image.h);
  for (unsigned int x=0; x<map.w; x++)
    for (unsigned int y=0; y<map.h; y++)
      map(x,y)=(image(x,y)) ? 1 : 0;

  for (unsigned int x=0; x<map.w; x++)
    for (unsigned int y=0; y<map.h; y++)
      if (map(x,y) > 0)
	for (unsigned int border=0; border < 4; border++)
	  if (Start(map,x,y,border)) {
	    int xx=x;
	    int yy=y;
	    int bborder=border;
	    Contour* current=new Contour();
	    contours.push_back(current);
	    do {
	      current->push_back(std::pair<unsigned int, unsigned int>(xx, yy));
	    } while (Step(map, xx, yy, bborder));
	  }
}

Contours::~Contours()
{
  for (unsigned int i=0; i<contours.size(); i++)
    delete contours[i];
}
