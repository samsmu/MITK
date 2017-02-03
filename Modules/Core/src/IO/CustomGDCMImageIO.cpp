#include "CustomGDCMImageIO.h"
#include <gdcmImageReader.h>
#include <gdcmImageHelper.h>
#include <gdcmRescaler.h>
#include <gdcmGlobal.h>
#include <gdcmDict.h>
#include <vnl/vnl_cross.h>
#include <itkMetaDataDictionary.h>
#include <gdcmStringFilter.h>
#include <gdcmDataSetHelper.h>
#include <gdcmUIDGenerator.h>
#include <itksys/Base64.h>
#include <itkMetaDataObject.h>
#include <itkByteSwapper.h>
#include <gdcmImageChangePlanarConfiguration.h>
#include <gdcmImageApplyLookupTable.h>
#include <gdcmImageRegionReader.h>
#include <gdcmBoxRegion.h>

CustomGDCMImageIO::CustomGDCMImageIO() :
  m_oneSlice(false)
{
}

CustomGDCMImageIO::~CustomGDCMImageIO()
{
}

bool CustomGDCMImageIO::CanReadFile(const char* filename)
{
  m_stream.open(filename, std::ios::in|std::ios::out|std::ios::binary);
  if (m_stream.fail())
  {
    m_stream.clear();
    m_stream.close();
    return false;
  }

  //
  // sniff for the DICM signature first at 128
  // then at zero, and if either place has it then
  // ask GDCM to try reading it.
  //
  // There isn't a definitive way to check for DICOM files;
  // This was actually cribbed from DICOMParser in VTK
  bool dicomsig = false;
  for(long int off = 128; off >= 0; off -= 128)
  {
    m_stream.seekg(off, std::ios_base::beg);
    if(m_stream.fail() || m_stream.eof())
    {
      m_stream.clear();
      m_stream.close();
      return false;
    }

    char buf[5];
    m_stream.read(buf,4);

    if(m_stream.fail())
    {
      m_stream.clear();
      m_stream.close();
      return false;
    }

    buf[4] = '\0';
    std::string sig(buf);
    if(sig == "DICM")
    {
      dicomsig = true;
    }
  }

  if(!dicomsig)
  {
    m_stream.seekg(0,std::ios_base::beg);
    unsigned short groupNo;

    m_stream.read(reinterpret_cast<char*>(&groupNo), sizeof(unsigned short));

    itk::ByteSwapper<unsigned short>::SwapFromSystemToLittleEndian(&groupNo);
    if(groupNo == 0x0002 || groupNo == 0x0008)
    {
        dicomsig = true;
    }
  }

  if(dicomsig)
  {
    // Check to see if its a valid dicom file gdcm is able to parse:
    // We are parsing the header one time here:
    gdcm::ImageReader reader;
    reader.SetStream(m_stream);
    if (reader.Read())
    {
      m_stream.close();
      return true;
    }
  }

  m_stream.close();
  return false;
}


void CustomGDCMImageIO::ReadImageInformation()
{
  // ensure file can be opened for reading, before doing any more work
  m_stream.open(m_FileName, std::ios::in|std::ios::out|std::ios::binary);
  if (m_stream.fail())
  {
    m_stream.clear();
    m_stream.close();
    return;
  }

  // In general this should be relatively safe to assume
  gdcm::ImageHelper::SetForceRescaleInterceptSlope(true);

  gdcm::ImageReader reader;
  reader.SetStream(m_stream);
  if (!reader.Read())
  {
    itkExceptionMacro(<< "Cannot read requested file");
  }

  const gdcm::Image& image = reader.GetImage();
  const gdcm::File& f = reader.GetFile();
  const gdcm::DataSet& ds = f.GetDataSet();
  const unsigned int* dims = image.GetDimensions();
  const gdcm::PixelFormat& pixeltype = image.GetPixelFormat();

  m_RescaleIntercept = image.GetIntercept();
  m_RescaleSlope = image.GetSlope();
  gdcm::Rescaler r;
  r.SetIntercept(m_RescaleIntercept);
  r.SetSlope(m_RescaleSlope);
  r.SetPixelFormat(pixeltype);
  gdcm::PixelFormat::ScalarType outputpt = r.ComputeInterceptSlopePixelType();
  
  itkAssertInDebugAndIgnoreInReleaseMacro(pixeltype <= outputpt);

  m_ComponentType = UNKNOWNCOMPONENTTYPE;
  switch (outputpt)
  {
  case gdcm::PixelFormat::INT8:
    m_ComponentType = ImageIOBase::CHAR; // Is it signed char ?
    break;
  case gdcm::PixelFormat::UINT8:
    m_ComponentType = ImageIOBase::UCHAR;
    break;
      /* INT12 / UINT12 should not happen anymore in any modern DICOM */
  case gdcm::PixelFormat::INT12:
    m_ComponentType = ImageIOBase::SHORT;
    break;
  case gdcm::PixelFormat::UINT12:
    m_ComponentType = ImageIOBase::USHORT;
    break;
  case gdcm::PixelFormat::INT16:
    m_ComponentType = ImageIOBase::SHORT;
    break;
  case gdcm::PixelFormat::UINT16:
    m_ComponentType = ImageIOBase::USHORT;
    break;
      // RT / SC have 32bits
  case gdcm::PixelFormat::INT32:
    m_ComponentType = ImageIOBase::INT;
    break;
  case gdcm::PixelFormat::UINT32:
    m_ComponentType = ImageIOBase::UINT;
    break;
      //case gdcm::PixelFormat::FLOAT16: // TODO
  case gdcm::PixelFormat::FLOAT32:
    m_ComponentType = ImageIOBase::FLOAT;
    break;
  case gdcm::PixelFormat::FLOAT64:
    m_ComponentType = ImageIOBase::DOUBLE;
    break;
  default:
    itkExceptionMacro("Unhandled PixelFormat: " << outputpt);
  }

  m_NumberOfComponents = pixeltype.GetSamplesPerPixel();
  if (image.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::PALETTE_COLOR)
  {
    itkAssertInDebugAndIgnoreInReleaseMacro(m_NumberOfComponents == 1);
    // TODO: need to do the LUT ourself...
    //itkExceptionMacro(<< "PALETTE_COLOR is not implemented yet");
    // AFAIK ITK user don't care about the palette so always apply it and fake a
    // RGB image for them
    m_NumberOfComponents = 3;
  }
  
  if (m_NumberOfComponents == 1)
  {
    this->SetPixelType(SCALAR);
  }
  else
  {
    this->SetPixelType(RGB); // What if image is YBR ? This is a problem since
      // integer conversion is lossy
  }

  // set values in case we don't find them
  //this->SetNumberOfDimensions(  image.GetNumberOfDimensions() );
  m_Dimensions[0] = dims[0];
  m_Dimensions[1] = dims[1];

  double spacing[3];

  //
  //
  // This is a WORKAROUND for a bug in GDCM -- in
  // ImageHeplper::GetSpacingTagFromMediaStorage it was not
  // handling some MediaStorage types
  // so we have to punt here.
  gdcm::MediaStorage ms;

  ms.SetFromFile(f);
  switch(ms)
  {
  case gdcm::MediaStorage::HardcopyGrayscaleImageStorage:
  case gdcm::MediaStorage::GEPrivate3DModelStorage:
  case gdcm::MediaStorage::Philips3D:
  case gdcm::MediaStorage::VideoEndoscopicImageStorage:
  case gdcm::MediaStorage::UltrasoundMultiFrameImageStorage:
  case gdcm::MediaStorage::UltrasoundImageStorage: // ??
  case gdcm::MediaStorage::UltrasoundImageStorageRetired:
  case gdcm::MediaStorage::UltrasoundMultiFrameImageStorageRetired:
  {
    std::vector<double> sp;
    gdcm::Tag spacingTag(0x0028,0x0030);
    if(ds.FindDataElement(spacingTag) && !ds.GetDataElement(spacingTag).IsEmpty())
    {
      gdcm::DataElement de = ds.GetDataElement(spacingTag);
      const gdcm::Global &g = gdcm::GlobalInstance;
      const gdcm::Dicts &dicts = g.GetDicts();
      const gdcm::DictEntry &entry = dicts.GetDictEntry(de.GetTag());
      const gdcm::VR & vr = entry.GetVR();

      switch(vr)
      {
      case gdcm::VR::DS:
      {
        std::stringstream m_Ss;

        gdcm::Element<gdcm::VR::DS,gdcm::VM::VM1_n> m_El;

        const gdcm::ByteValue* bv = de.GetByteValue();
        assert(bv);

        std::string s = std::string(bv->GetPointer(), bv->GetLength());
        m_Ss.str(s);

        // Stupid file: CT-MONO2-8-abdo.dcm
        // The spacing is something like that: [0.2\0\0.200000]
        // I would need to throw an expection that VM is not compatible
        m_El.SetLength(entry.GetVM().GetLength() * entry.GetVR().GetSizeof());
        m_El.Read(m_Ss);

        assert(m_El.GetLength() == 2);
        for(unsigned long i = 0; i < m_El.GetLength(); ++i)
        {
          sp.push_back(m_El.GetValue(i));
        }

        std::swap(sp[0], sp[1]);
        assert(sp.size() == static_cast<unsigned int>(entry.GetVM()));
      }
        break;
      case gdcm::VR::IS:
      {
        std::stringstream m_Ss;

        gdcm::Element<gdcm::VR::IS,gdcm::VM::VM1_n> m_El;

        const gdcm::ByteValue* bv = de.GetByteValue();
        assert(bv);

        std::string s = std::string(bv->GetPointer(), bv->GetLength());
        m_Ss.str(s);
        m_El.SetLength(entry.GetVM().GetLength() * entry.GetVR().GetSizeof());
        m_El.Read(m_Ss);

        for(unsigned long i = 0; i < m_El.GetLength(); ++i)
        {
          sp.push_back(m_El.GetValue(i));
        }
        assert(sp.size() == static_cast<unsigned int>(entry.GetVM()));
      }
        break;
      default:
        assert(0);
        break;
      }
      spacing[0] = sp[0];
      spacing[1] = sp[1];
      spacing[2] = 1.0; // punt?
    }
    else
    {
      spacing[0] = 1.0;
      spacing[1] = 1.0;
      spacing[2] = 1.0; // punt?
    }
  }
    break;
  default:
  {
    const double *sp;
    sp = image.GetSpacing();
    spacing[0] = sp[0];
    spacing[1] = sp[1];
    spacing[2] = sp[2];
  }
  break;
  }
  
  const double* origin = image.GetOrigin();
  for(unsigned i = 0; i < 3; ++i)
  {
    m_Spacing[i] = spacing[i];
    m_Origin[i] = origin[i];
  }

  if (image.GetNumberOfDimensions() == 3)
  {
    m_Dimensions[2] = dims[2];
  }
  else
  {
    m_Dimensions[2] = 1;
  }

  const double* dircos = image.GetDirectionCosines();
  vnl_vector<double> rowDirection(3), columnDirection(3);
  rowDirection[0] = dircos[0];
  rowDirection[1] = dircos[1];
  rowDirection[2] = dircos[2];

  columnDirection[0] = dircos[3];
  columnDirection[1] = dircos[4];
  columnDirection[2] = dircos[5];

  vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, columnDirection);

  // orthogonalize
  sliceDirection.normalize();
  rowDirection = vnl_cross_3d(columnDirection,sliceDirection).normalize();
  columnDirection = vnl_cross_3d(sliceDirection,rowDirection);

  this->SetDirection(0, rowDirection);
  this->SetDirection(1, columnDirection);
  this->SetDirection(2, sliceDirection);

  //Now copying the gdcm dictionary to the itk dictionary:
  itk::MetaDataDictionary & dico = this->GetMetaDataDictionary();

  // in the case of re-use by ImageSeriesReader, clear the dictionary
  // before populating it.
  dico.Clear();

  gdcm::StringFilter sf;
  sf.SetFile(f);

  // Copy of the header->content
  gdcm::DataSet::ConstIterator it = ds.Begin();
  for (; it != ds.End(); ++it)
  {
    const gdcm::DataElement& ref = *it;
    const gdcm::Tag& tag = ref.GetTag();
    // Compute VR from the toplevel file, and the currently processed dataset:
    gdcm::VR vr = gdcm::DataSetHelper::ComputeVR(f, ds, tag);

    // Process binary field and encode them as mime64: only when we do not know
    // of any better
    // representation. VR::US is binary, but user want ASCII representation.
    if (vr & (gdcm::VR::OB | gdcm::VR::OF | gdcm::VR::OW | gdcm::VR::SQ | gdcm::VR::UN))
    {
      /*
      * Old behavior was to skip SQ, Pixel Data element. I decided that it is not safe to mime64
      * VR::UN element. There used to be a bug in gdcm 1.2.0 and VR:UN element.
      */
      if ((m_LoadPrivateTags || tag.IsPublic()) && vr != gdcm::VR::SQ && tag != gdcm::Tag(0x7fe0, 0x0010))
      {
        const gdcm::ByteValue* bv = ref.GetByteValue();
        if (bv)
        {
          // base64 streams have to be a multiple of 4 bytes in length
          int encodedLengthEstimate = 2 * bv->GetLength();
          encodedLengthEstimate = ((encodedLengthEstimate / 4) + 1) * 4;

          char* bin = new char[encodedLengthEstimate];
          unsigned int encodedLengthActual = static_cast<unsigned int>(
                      itksysBase64_Encode(
                          (const unsigned char*)bv->GetPointer(),
                          static_cast<SizeValueType>(bv->GetLength()),
                          (unsigned char*)bin,
                          static_cast<int>(0)));
          std::string encodedValue(bin, encodedLengthActual);
          itk::EncapsulateMetaData<std::string>(dico, tag.PrintAsPipeSeparatedString(), encodedValue);
          delete[] bin;
        }
      }
    }
    else /* if ( vr & gdcm::VR::VRASCII ) */
    {
      // Only copying field from the public DICOM dictionary
      if (m_LoadPrivateTags || tag.IsPublic())
      {
        itk::EncapsulateMetaData<std::string>(dico, tag.PrintAsPipeSeparatedString(), sf.ToString(tag));
      }
    }
  }
  
  m_stream.close();

#if defined( ITKIO_DEPRECATED_GDCM1_API )
    // Now is a good time to fill in the class member:
    char name[512];
    this->GetPatientName(name);
    this->GetPatientID(name);
    this->GetPatientSex(name);
    this->GetPatientAge(name);
    this->GetStudyID(name);
    this->GetPatientDOB(name);
    this->GetStudyDescription(name);
    this->GetBodyPart(name);
    this->GetNumberOfSeriesInStudy(name);
    this->GetNumberOfStudyRelatedSeries(name);
    this->GetStudyDate(name);
    this->GetModality(name);
    this->GetManufacturer(name);
    this->GetInstitution(name);
    this->GetModel(name);
    this->GetScanOptions(name);
#endif
}

void CustomGDCMImageIO::Read(void *pointer)
{
  m_stream.open(m_FileName, std::ios::in|std::ios::out|std::ios::binary);
  if (m_stream.fail()) {
    m_stream.clear();
    m_stream.close();
    return;
  }
  
  itkAssertInDebugAndIgnoreInReleaseMacro(gdcm::ImageHelper::GetForceRescaleInterceptSlope());
  gdcm::ImageRegionReader reader;
  reader.SetStream(m_stream);
  if (!reader.ReadInformation())
  {
    itkExceptionMacro(<< "Cannot read requested file");
  }

  std::vector<unsigned int> dims = gdcm::ImageHelper::GetDimensionsValue(reader.GetFile());

  gdcm::BoxRegion region;

  if (m_oneSlice)
  {
    region.SetDomain(0, dims[0] - 1, 0, dims[1] - 1, 0, 0);
  }
  else
  {
    region.SetDomain(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  }

  reader.SetRegion(region);

  const size_t buflen = region.Area() * gdcm::ImageHelper::GetPixelFormatValue(reader.GetFile()).GetPixelSize();
    
  std::vector<char> bufferVec;
  bufferVec.resize(buflen);
  char* buffer = &bufferVec[0];

  if (!reader.ReadIntoBuffer(buffer, buflen))
  {
    itkExceptionMacro(<< "Cannot read requested file");
  }

  gdcm::Image & image = reader.GetImage();

  if (m_oneSlice)
  {
    image.SetNumberOfDimensions(2);
    image.SetDimension(0, dims[0]);
    image.SetDimension(1, dims[1]);
  }
  
  gdcm::DataElement pixeldata(gdcm::Tag(0x7fe0,0x0010));
  pixeldata.SetByteValue(buffer, (uint32_t)buflen);
  image.SetDataElement(pixeldata);

#ifndef NDEBUG
  gdcm::PixelFormat pixeltype_debug = image.GetPixelFormat();
  itkAssertInDebugAndIgnoreInReleaseMacro(image.GetNumberOfDimensions() == 2 || image.GetNumberOfDimensions() == 3);
#endif

  SizeValueType len = image.GetBufferLength();

  // I think ITK only allow RGB image by pixel (and not by plane)
  if (image.GetPlanarConfiguration() == 1)
  {
    gdcm::ImageChangePlanarConfiguration icpc;
    icpc.SetInput(image);
    icpc.SetPlanarConfiguration(0);
    icpc.Change();
    image = icpc.GetOutput();
  }

  gdcm::PhotometricInterpretation pi = image.GetPhotometricInterpretation();
  if (pi == gdcm::PhotometricInterpretation::PALETTE_COLOR)
  {
    gdcm::ImageApplyLookupTable ialut;
    ialut.SetInput(image);
    ialut.Apply();
    image = ialut.GetOutput();
    len *= 3;
  }

  if (!image.GetBuffer((char*)pointer))
  {
    itkExceptionMacro(<< "Failed to get the buffer!");
  }

  const gdcm::PixelFormat & pixeltype = image.GetPixelFormat();

#ifndef NDEBUG
  if (pi != gdcm::PhotometricInterpretation::PALETTE_COLOR)
  {
    itkAssertInDebugAndIgnoreInReleaseMacro(pixeltype_debug == pixeltype);
  }
#endif

  if (m_RescaleSlope != 1.0 || m_RescaleIntercept != 0.0)
  {
    gdcm::Rescaler r;
    r.SetIntercept(m_RescaleIntercept);
    r.SetSlope(m_RescaleSlope);
    r.SetPixelFormat(pixeltype);

    gdcm::PixelFormat outputpt = r.ComputeInterceptSlopePixelType();
    
    char* copy = new char[len];
    memcpy(copy, (char*)pointer, len);
    r.Rescale((char*)pointer, copy, len);
    delete[] copy;
    // WARNING: sizeof(Real World Value) != sizeof(Stored Pixel)
    len = len * outputpt.GetPixelSize() / pixeltype.GetPixelSize();
  }
  
  m_stream.close();
}




