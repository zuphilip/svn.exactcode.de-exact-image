#include "ContourUtility.hh"
#include <math.h>
//#include <iostream>

void CenterAndReduce(const Contours::Contour& source,
		     Contours::Contour& dest,
		     unsigned int shift, // integer coordinate bit reduction
		     double& drx, // returned centroid
		     double& dry  // returned centroid
		     )
{
  unsigned int rx=0;
  unsigned int ry=0;
  unsigned int lastx=(unsigned int)-1;
  unsigned int lasty=(unsigned int)-1;
  for (unsigned int i=0; i<source.size(); i++) {
    unsigned int x=(int)source[i].first >> shift;
    unsigned int y=(int)source[i].second >> shift;
    if (x != lastx || y != lasty) {
      dest.push_back(std::pair<unsigned int, unsigned int>(x, y));
      lastx=x;
      lasty=y;
      rx+=x;
      ry+=y;
    }
  }
  drx=((double) rx / (double) dest.size());
  dry=((double) ry / (double) dest.size());
}


void RotCenterAndReduce(const Contours::Contour& source,
			Contours::Contour& dest,
			double phi, // clockwise rotation in radians
			unsigned int add, // added to all coordinates to avoid negative values,
                                          // should be to set to image.w+image.h
			unsigned int shift, // integer coordinate bit reduction
			double& drx, // returned centroid 
			double& dry  // returned centroid
			)
{
  Contours::Contour tmp;

  double c=cos(phi);
  double s=sin(phi);

  int lastx=0;
  int lasty=0;
  for (unsigned int i=0; i<source.size(); i++) {
    double dx=(double)source[i].first;
    double dy=(double)source[i].second;

    double nx=c*dx - s*dy;
    double ny=s*dx + c*dy;

    int x=(int)nx + (int)add;
    int y=(int)ny + (int)add;

    // in case of gab, place an intermediate contour pixel
    if (i > 0 && (abs(x-lastx) > 1 || abs(y-lasty) > 1)) {
      tmp.push_back(std::pair<unsigned int, unsigned int>((x+lastx)/2, (y+lasty)/2));
    }

    tmp.push_back(std::pair<unsigned int, unsigned int>(x,y));
    lastx=x;
    lasty=y;
  }

  CenterAndReduce(tmp, dest, shift, drx, dry);
}

double L1Dist(const Contours::Contour& a,
	      const Contours::Contour& b,
	      double drax, // a centroid
	      double dray, // a centroid
	      double drbx, // b centroid
	      double drby, // b centroid
	      unsigned int shift,   // reduction bits (in case a and b are reduced) or 0
	      double& transx, // returned translation for a
	      double& transy  // returned translation for a
	      )
{
  double factor=(double)(1 << shift);
  transx= (drbx-drax)*factor;
  transy= (drby-dray)*factor;

  int dx= (int)(drbx-drax);
  int dy= (int)(drby-dray);
  double sum=.0;

  int best=1000000;
  int opt=0;
  int delta=0;
  int lastpos=0;
  //bool forward=true;
  for (unsigned int i=0; i<a.size(); i++) {
    if (i>0) {
      delta=abs((int)a[i].first-(int)a[i-1].first)+abs((int)a[i].second-(int)a[i-1].second);
      opt=best-delta;
      best+=delta;
    }

    int pos=lastpos;
    for (unsigned int j=0; j<b.size(); j++) {
      int current=abs(dx+(int)a[i].first-(int)b[pos].first)+abs(dy+(int)a[i].second-(int)b[pos].second);

      /* sanity check
      if (current < 0) {
	std::cout << "? " << current << "\t"
		  << abs(dx+(int)a[i].first-(int)b[pos].first) << "\t"
		  << abs(dy+(int)a[i].second-(int)b[pos].second) << std::endl;
	std::cout << dx << "\t" << (int)a[i].first << "\t" << (int)b[pos].first << std::endl;
	std::cout << dy << "\t" << (int)a[i].second << "\t" << (int)b[pos].second << std::endl;	
      } */

      if (current < best) {
	best=current;
	//forward=(pos>lastpos);
	lastpos=pos;
	if (best == opt)
	  j=b.size();
      }
      else if (current > best) {
	int skip=((current - best) - 1) / 2;
	j+=skip;
	pos+=skip; //(forward) ? skip : -skip;
      }

      //if (forward) {
	pos++;
	if (pos >= b.size())
	  pos-=b.size();
	//} else {
	//	pos--;
	//if (pos < 0)
	//  pos+=b.size();
	//}
    }

    sum += best;
  }
  
  return sum*factor;
}

// not very efficient, yet effective
static void PutPixel(Image& img, int x, int y, unsigned int R, unsigned int G, unsigned int B)
{
  Image::iterator color=img.begin();
  color.setRGB(R, G, B);

  Image::iterator p=img.begin();
  p=p.at(x,y);
  p.set(color);
}


void DrawContour(Image& img, const Contours::Contour& c, unsigned int r, unsigned int g, unsigned int b)
{
  for (unsigned int i=0; i<c.size(); i++)
    PutPixel(img, c[i].first, c[i].second, r, g, b);
}

void DrawTContour(Image& img, const Contours::Contour& c, unsigned int tx, unsigned int ty, unsigned int r, unsigned int g, unsigned int b)
{
  for (unsigned int i=0; i<c.size(); i++) {
    int x=c[i].first+tx;
    int y=c[i].second+ty;
    if (x >= 0 && x <= img.w && y >= 0 && y <= img.h)
      PutPixel(img, x, y, r, g, b);
  }
}



bool WriteContour(FILE* f, const Contours::Contour& source);
bool ReadContour(FILE* f, const Contours::Contour& dest);

