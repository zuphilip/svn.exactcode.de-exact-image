#include "FG-Matrix.hh"
#include <vector>

class Contours
{
public:
  typedef std::vector < std::pair<unsigned int, unsigned int> > Contour;
  typedef DataMatrix<int> VisitMap;

  Contours(const FGMatrix& image);
  ~Contours();

  std::vector <Contour*> contours;
};
