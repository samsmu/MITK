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

#include "mitkNodePredicateDimensionOnly.h"
#include "mitkImage.h"
#include "mitkDataNode.h"


mitk::NodePredicateDimensionOnly::NodePredicateDimensionOnly(unsigned int dimension)
: m_Dimension( dimension )
{
}

mitk::NodePredicateDimensionOnly::~NodePredicateDimensionOnly()
{
}

bool mitk::NodePredicateDimensionOnly::CheckNode(const mitk::DataNode* node) const
{
  if (node == nullptr) {
    throw std::invalid_argument("NodePredicateDimension: invalid node");
  }

  mitk::Image *image = dynamic_cast<mitk::Image *>( node->GetData() );
  if (image != nullptr) {
    return image->GetDimension() == m_Dimension;
  }

  return false;
}

