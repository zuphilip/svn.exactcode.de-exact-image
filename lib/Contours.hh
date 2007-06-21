#include "FG-Matrix.hh"
#include <vector>

class Contours
{
public:
  typedef std::vector < std::pair<unsigned int, unsigned int> > Contour;
  typedef DataMatrix<int> VisitMap;

  Contours(const FGMatrix& image);
  Contours() {} // empty constructor for generic usage

  ~Contours();

  std::vector <Contour*> contours;
};
