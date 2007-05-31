#include "Image.hh"
#include "DataMatrix.hh"

class FGMatrix : public DataMatrix<bool>
{
public:
  FGMatrix(Image& image, unsigned int fg_threshold);
  FGMatrix(const FGMatrix& source, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  ~FGMatrix();
};
  
