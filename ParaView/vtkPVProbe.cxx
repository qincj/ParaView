/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVProbe.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVProbe.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkPVSourceInterface.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  this->CommandFunction = vtkPVProbeCommand;

  this->DimensionalityMenu = vtkKWOptionMenu::New();
  this->DimensionalityLabel = vtkKWLabel::New();
  this->SelectPointButton = vtkKWPushButton::New();
  
  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->SelectedXEntry = vtkKWLabeledEntry::New();
  this->SelectedYEntry = vtkKWLabeledEntry::New();
  this->SelectedZEntry = vtkKWLabeledEntry::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->EndPointMenuFrame = vtkKWWidget::New();
  this->EndPointLabel = vtkKWLabel::New();
  this->EndPointMenu = vtkKWOptionMenu::New();

  this->EndPoint1Frame = vtkKWWidget::New();
  this->EndPoint1Label = vtkKWLabel::New();
  this->End1XEntry = vtkKWLabeledEntry::New();
  this->End1YEntry = vtkKWLabeledEntry::New();
  this->End1ZEntry = vtkKWLabeledEntry::New();
  
  this->EndPoint2Frame = vtkKWWidget::New();
  this->EndPoint2Label = vtkKWLabel::New();
  this->End2XEntry = vtkKWLabeledEntry::New();
  this->End2YEntry = vtkKWLabeledEntry::New();
  this->End2ZEntry = vtkKWLabeledEntry::New();
  
  this->DivisionsEntry = vtkKWLabeledEntry::New();
  
  this->SetPointButton = vtkKWPushButton::New();
  
  this->ProbeFrame = vtkKWWidget::New();

  this->Interactor = NULL;
  
  this->SelectedPoint[0] = this->SelectedPoint[1] = this->SelectedPoint[2] = 0;
  this->EndPoint1[0] = -0.5;
  this->EndPoint1[1] = this->EndPoint1[2] = 0;
  this->EndPoint2[0] = 0.5;
  this->EndPoint2[1] = this->EndPoint2[2] = 0;

  this->ProbeSourceTclName = NULL;
  this->Dimensionality = -1;
  
  this->PVProbeSource = NULL;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{
  this->DimensionalityLabel->Delete();
  this->DimensionalityLabel = NULL;
  this->DimensionalityMenu->Delete();
  this->DimensionalityMenu = NULL;
  
  this->SelectPointButton->Delete();
  this->SelectPointButton = NULL;
  
  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedXEntry->Delete();
  this->SelectedXEntry = NULL;
  this->SelectedYEntry->Delete();
  this->SelectedYEntry = NULL;
  this->SelectedZEntry->Delete();
  this->SelectedZEntry = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;

  this->EndPointLabel->Delete();
  this->EndPointLabel = NULL;
  this->EndPointMenu->Delete();
  this->EndPointMenu = NULL;
  this->EndPointMenuFrame->Delete();
  this->EndPointMenuFrame = NULL;

  this->EndPoint1Label->Delete();
  this->EndPoint1Label = NULL;
  this->End1XEntry->Delete();
  this->End1XEntry = NULL;
  this->End1YEntry->Delete();
  this->End1YEntry = NULL;
  this->End1ZEntry->Delete();
  this->End1ZEntry = NULL;
  this->EndPoint1Frame->Delete();
  this->EndPoint1Frame = NULL;
  
  this->EndPoint2Label->Delete();
  this->EndPoint2Label = NULL;
  this->End2XEntry->Delete();
  this->End2XEntry = NULL;
  this->End2YEntry->Delete();
  this->End2YEntry = NULL;
  this->End2ZEntry->Delete();
  this->End2ZEntry = NULL;
  this->EndPoint2Frame->Delete();
  this->EndPoint2Frame = NULL;
  
  this->DivisionsEntry->Delete();
  this->DivisionsEntry = NULL;

  this->SetPointButton->Delete();
  this->SetPointButton = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;

  this->SetPVProbeSource(NULL);
}

//----------------------------------------------------------------------------
vtkPVProbe* vtkPVProbe::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVProbe");
  if (ret)
    {
    return (vtkPVProbe*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVProbe();
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  float bounds[6];
  vtkKWWidget *frame;
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();

  frame = vtkKWWidget::New();
  frame->SetParent(this->GetParameterFrame()->GetFrame());
  frame->Create(pvApp, "frame", "");
  
  this->DimensionalityLabel->SetParent(frame);
  this->DimensionalityLabel->Create(pvApp, "");
  this->DimensionalityLabel->SetLabel("Probe Dimensionality:");
  
  this->DimensionalityMenu->SetParent(frame);
  this->DimensionalityMenu->Create(pvApp, "");
  this->DimensionalityMenu->AddEntryWithCommand("Point", this, "UsePoint");
  this->DimensionalityMenu->AddEntryWithCommand("Line", this, "UseLine");
  this->DimensionalityMenu->SetValue("Point");
  
  this->Script("pack %s %s -side left",
               this->DimensionalityLabel->GetWidgetName(),
               this->DimensionalityMenu->GetWidgetName());
  
  this->SelectPointButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->SelectPointButton->Create(pvApp, "-text \"3D Cursor\"");
  this->SelectPointButton->SetCommand(this, "SetInteractor");
  
  this->SetPointButton->SetParent(this->ParameterFrame->GetFrame());
  this->SetPointButton->Create(pvApp, "");
  this->SetPointButton->SetLabel("Set Point");
  this->SetPointButton->SetCommand(this, "SetPoint");
  
  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s %s %s %s",
               frame->GetWidgetName(),
               this->SelectPointButton->GetWidgetName(),
               this->SetPointButton->GetWidgetName(),
               this->ProbeFrame->GetWidgetName());
  
  frame->Delete();

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");
  this->SelectedXEntry->SetParent(this->SelectedPointFrame);
  this->SelectedXEntry->Create(pvApp);
  this->SelectedXEntry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->SelectedXEntry->GetEntry()->GetWidgetName());
  this->SelectedXEntry->SetValue(this->SelectedPoint[0], 4);
  this->SelectedYEntry->SetParent(this->SelectedPointFrame);
  this->SelectedYEntry->Create(pvApp);
  this->SelectedYEntry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->SelectedYEntry->GetEntry()->GetWidgetName());
  this->SelectedYEntry->SetValue(this->SelectedPoint[1], 4);
  this->SelectedZEntry->SetParent(this->SelectedPointFrame);
  this->SelectedZEntry->Create(pvApp);
  this->SelectedZEntry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->SelectedZEntry->GetEntry()->GetWidgetName());
  this->SelectedZEntry->SetValue(this->SelectedPoint[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->SelectedPointLabel->GetWidgetName(),
               this->SelectedXEntry->GetWidgetName(),
               this->SelectedYEntry->GetWidgetName(),
               this->SelectedZEntry->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  // widgets for lines
  this->EndPointMenuFrame->SetParent(this->ProbeFrame);
  this->EndPointMenuFrame->Create(pvApp, "frame", "");
  this->EndPointLabel->SetParent(this->EndPointMenuFrame);
  this->EndPointLabel->Create(pvApp, "");
  this->EndPointLabel->SetLabel("Select End Point:");
  this->EndPointMenu->SetParent(this->EndPointMenuFrame);
  this->EndPointMenu->Create(pvApp, "");
  this->EndPointMenu->AddEntry("End Point 1");
  this->EndPointMenu->AddEntry("End Point 2");
  this->EndPointMenu->SetValue("End Point 1");
  
  this->Script("pack %s %s -side left", this->EndPointLabel->GetWidgetName(),
               this->EndPointMenu->GetWidgetName());

  this->EndPoint1Frame->SetParent(this->ProbeFrame);
  this->EndPoint1Frame->Create(pvApp, "frame", "");
  
  this->EndPoint1Label->SetParent(this->EndPoint1Frame);
  this->EndPoint1Label->Create(pvApp, "");
  this->EndPoint1Label->SetLabel("End Point 1");
  this->End1XEntry->SetParent(this->EndPoint1Frame);
  this->End1XEntry->Create(pvApp);
  this->End1XEntry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->End1XEntry->GetEntry()->GetWidgetName());
  this->End1XEntry->SetValue(this->EndPoint1[0], 4);
  this->End1YEntry->SetParent(this->EndPoint1Frame);
  this->End1YEntry->Create(pvApp);
  this->End1YEntry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->End1YEntry->GetEntry()->GetWidgetName());
  this->End1YEntry->SetValue(this->EndPoint1[1], 4);
  this->End1ZEntry->SetParent(this->EndPoint1Frame);
  this->End1ZEntry->Create(pvApp);
  this->End1ZEntry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->End1ZEntry->GetEntry()->GetWidgetName());
  this->End1ZEntry->SetValue(this->EndPoint1[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->EndPoint1Label->GetWidgetName(),
               this->End1XEntry->GetWidgetName(),
               this->End1YEntry->GetWidgetName(),
               this->End1ZEntry->GetWidgetName());
  
  this->EndPoint2Frame->SetParent(this->ProbeFrame);
  this->EndPoint2Frame->Create(pvApp, "frame", "");
  
  this->EndPoint2Label->SetParent(this->EndPoint2Frame);
  this->EndPoint2Label->Create(pvApp, "");
  this->EndPoint2Label->SetLabel("End Point 2");
  this->End2XEntry->SetParent(this->EndPoint2Frame);
  this->End2XEntry->Create(pvApp);
  this->End2XEntry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->End2XEntry->GetEntry()->GetWidgetName());
  this->End2XEntry->SetValue(this->EndPoint2[0], 4);
  this->End2YEntry->SetParent(this->EndPoint2Frame);
  this->End2YEntry->Create(pvApp);
  this->End2YEntry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->End2YEntry->GetEntry()->GetWidgetName());
  this->End2YEntry->SetValue(this->EndPoint2[1], 4);
  this->End2ZEntry->SetParent(this->EndPoint2Frame);
  this->End2ZEntry->Create(pvApp);
  this->End2ZEntry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->End2ZEntry->GetEntry()->GetWidgetName());
  this->End2ZEntry->SetValue(this->EndPoint2[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->EndPoint2Label->GetWidgetName(),
               this->End2XEntry->GetWidgetName(),
               this->End2YEntry->GetWidgetName(),
               this->End2ZEntry->GetWidgetName());
  
  this->DivisionsEntry->SetParent(this->ProbeFrame);
  this->DivisionsEntry->Create(pvApp);
  this->DivisionsEntry->SetLabel("Number of Line Divisions:");
  this->DivisionsEntry->SetValue(10);
  
  this->AcceptCommands->AddString("%s UpdateProbe",
                                  this->GetTclName());
  
  this->SetInteractor();
  
  this->PVProbeSource->GetBounds(bounds);
  this->Interactor->SetBounds(bounds);
  
  this->Script("grab release %s", this->ParameterFrame->GetWidgetName());

  this->UsePoint();
}

void vtkPVProbe::SetInteractor()
{
  if (!this->Interactor)
    {
    this->Interactor = 
      vtkKWSelectPointInteractor::SafeDownCast(this->GetWindow()->
                                               GetSelectPointInteractor());
    }
  
  this->GetWindow()->GetMainView()->SetInteractor(this->Interactor);
  this->Interactor->SetCursorVisibility(1);
}

void vtkPVProbe::AcceptCallback()
{
  // call the superclass's method
  this->vtkPVSource::AcceptCallback();
  
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numMenus, i;
  
  if (this->Dimensionality == 0)
    {
    // update the ui to see the point data for the probed point
    int error;
    vtkPolyData *probeOutput = (vtkPolyData *)
      (vtkTclGetPointerFromObject(this->GetNthPVOutput(0)->GetVTKDataTclName(),
				  "vtkPolyData", pvApp->GetMainInterp(),
				  error));
    
    vtkPointData *pd = probeOutput->GetPointData();
    
    int numArrays = pd->GetNumberOfArrays();
    
    vtkIdType j, numComponents;
    vtkDataArray *array;
    char label[256]; // this may need to be longer
    char arrayData[256];
    char tempArray[32];
    
    sprintf(label, "");
    
    for (i = 0; i < numArrays; i++)
      {
      array = pd->GetArray(i);
      numComponents = array->GetNumberOfComponents();
      if (numComponents > 1)
        {
        sprintf(arrayData, "%s: (", array->GetName());
        for (j = 0; j < numComponents; j++)
          {
          sprintf(tempArray, "%f", array->GetComponent(0, j));
	  
          if (j < numComponents - 1)
            {
            strcat(tempArray, ",");
            if (j % 3 == 2)
              {
              strcat(tempArray, "\n\t");
              }
            else
              {
              strcat(tempArray, " ");
              }
            }
          else
            {
            strcat(tempArray, ")\n");
            }
          strcat(arrayData, tempArray);
          }
        strcat(label, arrayData);
        }
      else
        {
	sprintf(arrayData, "%s: %f\n", array->GetName(),
                array->GetComponent(0, 0));
        strcat(label, arrayData);
        }
      }
    this->PointDataLabel->SetLabel(label);
    }
  else if (this->Dimensionality == 1)
    {
    this->Script("vtkXYPlotActor xyplot");
    this->Script("xyplot AddInput [%s GetOutput]",
		 this->GetVTKSourceTclName());
    this->Script("[xyplot GetPositionCoordinate] SetValue 0.05 0.05 0");
    this->Script("[xyplot GetPosition2Coordinate] SetValue 0.8 0.3 0");
    this->Script("xyplot SetNumberOfXLabels 5");
    this->Script("%s AddActor xyplot",
		 this->GetWindow()->GetMainView()->GetRendererTclName());
    this->Script("xyplot Delete");
    }

  this->Interactor->SetCursorVisibility(0);
  
  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName());
}

void vtkPVProbe::UpdateProbe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (!pvApp)
    {
    vtkErrorMacro("No vtkPVApplication set");
    return;
    }

  vtkMultiProcessController *controller = pvApp->GetController();
  
  if (!controller)
    {
    vtkErrorMacro("No vtkMultiProcessController");
    return;
    }
  
  vtkPolyData *probeOutput, *remoteProbeOutput = vtkPolyData::New();
  int error, i, k, numProcs, numComponents;
  vtkIdType numPoints, numRemotePoints, j, pointId;
  vtkIdTypeArray *validPoints = vtkIdTypeArray::New();
  vtkPointData *pointData, *remotePointData;
  float *tuple;
  char *outputTclName;
  
  if (this->Dimensionality == 0)
    {
    pvApp->BroadcastScript("vtkPolyData probeInput");
    pvApp->BroadcastScript("vtkIdList pointList");
    pvApp->BroadcastScript("pointList Allocate 1 1");
    pvApp->BroadcastScript("pointList InsertNextId 0");
    pvApp->BroadcastScript("vtkPoints points");
    pvApp->BroadcastScript("points Allocate 1 1");
    pvApp->BroadcastScript("points InsertNextPoint %f %f %f",
                           this->SelectedXEntry->GetValueAsFloat(),
                           this->SelectedYEntry->GetValueAsFloat(),
                           this->SelectedZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("probeInput Allocate 1 1");
    pvApp->BroadcastScript("probeInput SetPoints points");
    pvApp->BroadcastScript("probeInput InsertNextCell 1 pointList");
    pvApp->BroadcastScript("%s SetInput probeInput",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("pointList Delete");
    pvApp->BroadcastScript("points Delete");
    pvApp->BroadcastScript("probeInput Delete");
    }
  else if (this->Dimensionality == 1)
    {
    pvApp->BroadcastScript("vtkLineSource line");
    pvApp->BroadcastScript("line SetPoint1 %f %f %f",
                           this->End1XEntry->GetValueAsFloat(),
                           this->End1YEntry->GetValueAsFloat(),
                           this->End1ZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetPoint2 %f %f %f",
                           this->End2XEntry->GetValueAsFloat(),
                           this->End2YEntry->GetValueAsFloat(),
                           this->End2ZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetResolution %d",
			   this->DivisionsEntry->GetValueAsInt());
    pvApp->BroadcastScript("%s SetInput [line GetOutput]",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("line Delete");
    }
  
  pvApp->BroadcastScript("%s SetSource %s",
                         this->GetVTKSourceTclName(),
                         this->ProbeSourceTclName);
  this->GetNthPVOutput(0)->Update();
  
  pvApp->BroadcastScript("Application SendProbeData %s",
			 this->GetVTKSourceTclName());

  outputTclName = this->GetNthPVOutput(0)->GetVTKDataTclName();
  
  probeOutput =
    (vtkPolyData*)(vtkTclGetPointerFromObject(outputTclName, "vtkPolyData",
					      pvApp->GetMainInterp(), error));
  pointData = probeOutput->GetPointData();  
  numPoints = probeOutput->GetNumberOfPoints();
  numProcs = controller->GetNumberOfProcesses();
  numComponents = pointData->GetNumberOfComponents();
  tuple = new float[numComponents];
  
  for (i = 1; i < numProcs; i++)
    {
    controller->Receive(&numRemotePoints, 1, i, 1970);
    if (numRemotePoints > 0)
      {
      controller->Receive(validPoints, i, 1971);
      controller->Receive(remoteProbeOutput, i, 1972);
      
      remotePointData = remoteProbeOutput->GetPointData();
      for (j = 0; j < numRemotePoints; j++)
	{
	pointId = validPoints->GetValue(j);
	
	remotePointData->GetTuple(pointId, tuple);
	
	for (k = 0; k < numComponents; k++)
	  {
	  this->Script("[%s GetPointData] SetComponent %d %d %f",
		       outputTclName, pointId, k, tuple[k]);
	  }
	}
      }
    }  

  delete [] tuple;
  validPoints->Delete();
  remoteProbeOutput->Delete();
}

void vtkPVProbe::Deselect(vtkKWView *view)
{
  this->Interactor->SetCursorVisibility(0);
  this->vtkPVSource::Deselect(view);
}

void vtkPVProbe::DeleteCallback()
{
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int i, numMenus;
  
  this->Interactor->SetCursorVisibility(0);
  
  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName()); 

  // call the superclass
  this->vtkPVSource::DeleteCallback();  
}

void vtkPVProbe::UsePoint()
{
  this->Dimensionality = 0;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s",
               this->SelectedPointFrame->GetWidgetName(),
               this->PointDataLabel->GetWidgetName());
}

void vtkPVProbe::UseLine()
{
  this->Dimensionality = 1;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s %s %s",
               this->EndPointMenuFrame->GetWidgetName(),
               this->EndPoint1Frame->GetWidgetName(),
               this->EndPoint2Frame->GetWidgetName(),
	       this->DivisionsEntry->GetWidgetName());
}

void vtkPVProbe::SetPoint()
{
  if (this->Dimensionality == 0)
    {
    this->Interactor->GetSelectedPoint(this->SelectedPoint);
    this->SelectedXEntry->SetValue(this->SelectedPoint[0], 4);
    this->SelectedYEntry->SetValue(this->SelectedPoint[1], 4);
    this->SelectedZEntry->SetValue(this->SelectedPoint[2], 4);
    }
  else if (this->Dimensionality == 1)
    {
    char *endPoint = this->EndPointMenu->GetValue();
    if (strcmp(endPoint, "End Point 1") == 0)
      {
      this->Interactor->GetSelectedPoint(this->EndPoint1);
      this->End1XEntry->SetValue(this->EndPoint1[0], 4);
      this->End1YEntry->SetValue(this->EndPoint1[1], 4);
      this->End1ZEntry->SetValue(this->EndPoint1[2], 4);
      }
    else if (strcmp(endPoint, "End Point 2") == 0)
      {
      this->Interactor->GetSelectedPoint(this->EndPoint2);
      this->End2XEntry->SetValue(this->EndPoint2[0], 4);
      this->End2YEntry->SetValue(this->EndPoint2[1], 4);
      this->End2ZEntry->SetValue(this->EndPoint2[2], 4);
      }
    }
}
