/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef PlyFileWriterService_h
#define PlyFileWriterService_h

#include <mitkAbstractFileWriter.h>

namespace mitk
{

/**
 * @brief
 *
 * @ingroup IOExt
 */
class PlyFileWriterService : public AbstractFileWriter
{
public:

  PlyFileWriterService();
  virtual ~PlyFileWriterService();

  using AbstractFileWriter::Write;
  virtual void Write();

private:

  PlyFileWriterService(const PlyFileWriterService& other);

  virtual mitk::PlyFileWriterService* Clone() const;

};

}

#endif
