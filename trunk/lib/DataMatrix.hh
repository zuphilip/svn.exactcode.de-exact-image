#ifndef DATA_MATRIX_HH__
#define DATA_MATRIX_HH__

template <typename T>
class DataMatrix
{
public:
  DataMatrix(unsigned int iw, unsigned int ih)
  {
    w=iw;
    h=ih;
    master=true;

    data=new T*[w];
    for (unsigned int x=0; x<w; x++)
      data[x]=new T[h];
  }

  DataMatrix(const DataMatrix<T>& source, unsigned int ix, unsigned int iy, unsigned int iw, unsigned int ih)
  {
    w=iw;
    h=ih;
    master=false;

    data=new T*[w];
    for (unsigned int x=0; x<w; x++)
      data[x]=source.data[x+ix]+iy;
  }
  
  ~DataMatrix()
  {
    if (master)
      for (unsigned int x=0; x<w; x++)
	delete[] data[x];
    delete[] data;
  }

  unsigned int w;
  unsigned int h;
  T** data;
  bool master;

  const T& operator() (unsigned int x, unsigned int y) const
  {
    return data[x][y];
  }

  T& operator() (unsigned int x, unsigned int y)
  {
    return data[x][y];
  }
};

#endif
  
