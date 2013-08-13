/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleRangeDomain.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMDoubleRangeDomain);
//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::vtkSMDoubleRangeDomain()
{
}

//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::~vtkSMDoubleRangeDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
