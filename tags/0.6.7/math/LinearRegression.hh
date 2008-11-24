#ifndef LINREG_HH
#define LINREG_HH

#include <utility> // std::pair
#include <limits>
#include <cmath>

#include <ostream>

template <typename T>
class LinearRegression
{
public:
  
  LinearRegression ()
    : n (), sumX (), sumY (), sumXsquared (), sumYsquared (), sumXY () {}
  
  void clear () {
    n = 0;
    sumX = T ();
    sumY = T ();
    sumXsquared = T ();
    sumYsquared = T ();
    sumXY = T ();
  }
  
  
  template <typename T1>
  friend std::ostream& operator<< (std::ostream& s, LinearRegression<T1>& v);
  
  void addXY (const T& x, const T& y, bool calc = true) {
    sumX += x;
    sumY += y;
    sumXsquared += x * x;
    sumYsquared += y * y;
    sumXY += x * y;
    ++n;
    calculate ();
  }
  
  void addPair (const std::pair<T, T>& p, bool calc = true) {
    addXY (p.first, p.second, calc);
  }
  
  template <class IT>
  void addRange (IT it, IT it_end) {
    for (; it != it_end; ++it)
      addPair (*it, false);
    calculate ();
  }

  bool haveData() const { return n > 2; }
  long size () const { return n; }

  double getA () const { return a; }
  double getB () const { return b; }

  double getCoefDeterm () const { return coefD; }
  double getCoefCorrel () const { return coefC; }
  double getStdErrorEst () const { return stdError; }
  double estimateY (const T& x) const { return (a + b * x); }

protected:
  long n;
  
  T sumX, sumY;
  T sumXsquared, sumYsquared;
  T sumXY;

  T a, b;
  T coefD, coefC,stdError;

  void calculate ()
  {
    a = b = coefD = coefC = stdError = 0;

    if (!haveData())
      return;
    
    if (std::abs (sumXsquared * n - sumX * sumX) > std::numeric_limits<T>::epsilon ())
      {
	b = (sumXY * n- sumY * sumX) /
	  ( sumXsquared * n - sumX * sumX);
	a = (sumY - b * sumX) / n;
	
	T sx = b * (sumXY - sumX * sumY / n);
	T sy2 = sumYsquared - sumY * sumY / n;
	T sy = sy2 - sx;
	
	coefD = sx / sy2;
	coefC = sqrt (coefD);
	stdError = sqrt (sy / (-2. + n));
      }
  }
};

template <typename T1>
std::ostream& operator<< (std::ostream& s, LinearRegression<T1>& v)
{
  if (v.haveData())
    s << "f(x) = " << v.getA() << " + " << v.getB() << " * x";
  return s;
}

#endif
