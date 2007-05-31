#include <vector>
#include "DataMatrix.hh"
#include "Image.hh"

class DistanceMatrix : public DataMatrix<unsigned int>
{
public:
  static const unsigned int precission_shift=3;
  static const unsigned int undefined_dist=(unsigned int) -1;

  DistanceMatrix(Image& image, unsigned int fg_threshold);
  DistanceMatrix(const DistanceMatrix& source, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  ~DistanceMatrix();
};
