#include "Contours.hh"

void CenterAndReduce(const Contours::Contour& source,
		     Contours::Contour& dest,
		     unsigned int shift, // integer coordinate bit reduction
		     double& drx, // returned centroid
		     double& dry  // returned centroid
		     );

void RotCenterAndReduce(const Contours::Contour& source,
			Contours::Contour& dest,
			double phi, // clockwise rotation in radians
			unsigned int add, // added to all coordinates to avoid negative values,
                                          // should be at least image diagonal length
			unsigned int shift, // integer coordinate bit reduction
			double& drx, // returned centroid 
			double& dry  // returned centroid
			);

// realy fast summation of minimum L1-distance
double L1Dist(const Contours::Contour& a,
	      const Contours::Contour& b,
	      double drxa, // a centroid
	      double drxb, // a centroid
	      double drya, // b centroid
	      double dryb, // b centroid
	      unsigned int shift,   // reduction bits (in case a and b are reduced) or 0
	      double& transx, // returned translation for a
	      double& transy  // returned translation for a
	      );

// not implemented yet, but could be usefull to cache preprocessed
// contours on disk
bool WriteContour(FILE* f, const Contours::Contour& source);
bool ReadContour(FILE* f, const Contours::Contour& dest);

