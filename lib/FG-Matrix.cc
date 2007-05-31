#include "FG-Matrix.hh"

FGMatrix::FGMatrix(Image& image, unsigned int fg_threshold)
  : DataMatrix<bool>(image.w, image.h)
{
  unsigned int line=0;
  unsigned int row=0;
  Image::iterator i=image.begin();
  Image::iterator end=image.end();
  for (; i!=end ; ++i) {
    data[row][line]=((*i).getL() < fg_threshold);

    if (++row == image.w) {
      line++;
      row=0;
    }
  }
}

FGMatrix::FGMatrix(const FGMatrix& source, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
  : DataMatrix<bool>(source, x,y,w,h)
{
}
  
FGMatrix::~FGMatrix()
{
}
