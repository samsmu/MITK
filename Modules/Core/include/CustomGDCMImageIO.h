#ifndef CUSTOMGDCMIMAGEIO_H
#define CUSTOMGDCMIMAGEIO_H

#include <itkGDCMImageIO.h>
//#include "istreamcache.h"

#include <fstream>

class CustomGDCMImageIO : public itk::GDCMImageIO
{
public:
  typedef CustomGDCMImageIO         Self;
  typedef itk::GDCMImageIO          Superclass;
  typedef itk::SmartPointer< Self > Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(CustomGDCMImageIO, Superclass);

  CustomGDCMImageIO();
  ~CustomGDCMImageIO();

  bool CanReadFile(const char *);
  void ReadImageInformation();
  void Read(void *buffer);

  void setOneSliceModeOn() { m_oneSlice = true; }

private:
  
  std::fstream m_stream;
  std::string m_filename;
  bool m_oneSlice;
};

#endif // CUSTOMGDCMIMAGEIO_H
