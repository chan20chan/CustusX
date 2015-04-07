/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/
#include <cxToolTipSampleWidget.h>

#include <QPushButton>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include "cxTypeConversions.h"
#include "cxLogger.h"
#include "cxVector3D.h"
#include "cxDefinitionStrings.h"
#include "cxLabeledComboBoxWidget.h"
#include "cxProfile.h"
#include "cxSelectDataStringProperty.h"
#include "cxSpaceProvider.h"
#include "cxPatientModelService.h"
#include "cxReporter.h"
#include "cxVisServices.h"
#include "cxStringPropertySelectTool.h"
#include "cxStringPropertySelectCoordinateSystem.h"

namespace cx
{

ToolTipSampleWidget::ToolTipSampleWidget(VisServicesPtr services, QWidget* parent) :
    BaseWidget(parent, "ToolTipSampleWidget", "ToolTip Sample"),
	mServices(services),
    mSampleButton(new QPushButton("Sample")),
    mSaveToFileNameLabel(new QLabel("<font color=red> No file selected </font>")),
    mSaveFileButton(new QPushButton("Save to...")),
	mTruncateFile(false)
{
  QVBoxLayout* toplayout = new QVBoxLayout(this);

  this->setToolTip("Sample the tool tip position");
  mCoordinateSystems = StringPropertySelectCoordinateSystem::New(services->getToolManager());
  mTools = StringPropertySelectTool::New(mServices->getToolManager());
	mData = StringPropertySelectData::New(mServices->getPatientService());

  mCoordinateSystemComboBox = new LabeledComboBoxWidget(this, mCoordinateSystems);
  mToolComboBox = new LabeledComboBoxWidget(this, mTools);
  mDataComboBox = new LabeledComboBoxWidget(this, mData);

  toplayout->addWidget(new QLabel("<b>Select coordinate system to sample in: </b>"));
  toplayout->addWidget(mCoordinateSystemComboBox);
  toplayout->addWidget(mToolComboBox);
  toplayout->addWidget(mDataComboBox);
  toplayout->addWidget(this->createHorizontalLine());
  toplayout->addWidget(mSampleButton);
  toplayout->addWidget(this->createHorizontalLine());
  toplayout->addWidget(mSaveFileButton);
  toplayout->addWidget(mSaveToFileNameLabel);
  toplayout->addStretch();

  connect(mSaveFileButton, SIGNAL(clicked()), this, SLOT(saveFileSlot()));
  connect(mSampleButton, SIGNAL(clicked()), this, SLOT(sampleSlot()));
  connect(mCoordinateSystems.get(), SIGNAL(changed()), this, SLOT(coordinateSystemChanged()));

  //setting initial state
  this->coordinateSystemChanged();
}

ToolTipSampleWidget::~ToolTipSampleWidget()
{}

void ToolTipSampleWidget::saveFileSlot()
{
  QString configPath = profile()->getPath();
  if(mServices->getPatientService()->isPatientValid())
	configPath = mServices->getPatientService()->getActivePatientFolder();

  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                             configPath+"/SampledPoints.txt",
                             tr("Text (*.txt)"));
  if(fileName.isEmpty())
    return;
  else if(QFile::exists(fileName))
    mTruncateFile = true;

  mSaveToFileNameLabel->setText(fileName);
}

void ToolTipSampleWidget::sampleSlot()
{
  QFile samplingFile(mSaveToFileNameLabel->text());

  CoordinateSystem to = this->getSelectedCoordinateSystem();
  Vector3D toolPoint = mServices->getSpaceProvider()->getActiveToolTipPoint(to, false);

  if(!samplingFile.open(QIODevice::WriteOnly | (mTruncateFile ? QIODevice::Truncate : QIODevice::Append)))
  {
    reportWarning("Could not open "+samplingFile.fileName());
    report("Sampled point: "+qstring_cast(toolPoint));
    return;
  }
  else
  {
    if(mTruncateFile)
      mTruncateFile = false;
  }

  QString sampledPoint = qstring_cast(toolPoint);

  QTextStream streamer(&samplingFile);
  streamer << sampledPoint;
  streamer << endl;

  reporter()->playSampleSound();
  report("Sampled point in "+qstring_cast(to.mId)+" ("+to.mRefObject+") space, result: "+sampledPoint);
}

void ToolTipSampleWidget::coordinateSystemChanged()
{
  switch (string2enum<COORDINATE_SYSTEM>(mCoordinateSystems->getValue()))
  {
  case csDATA:
    mDataComboBox->show();
    mToolComboBox->hide();
    break;
  case csTOOL:
    mToolComboBox->show();
    mDataComboBox->hide();
    break;
  case csSENSOR:
    mToolComboBox->show();
    mDataComboBox->hide();
    break;
  default:
    mDataComboBox->hide();
    mToolComboBox->hide();
    break;
  };
}

CoordinateSystem ToolTipSampleWidget::getSelectedCoordinateSystem()
{
  CoordinateSystem retval(csCOUNT);

  retval.mId = string2enum<COORDINATE_SYSTEM>(mCoordinateSystems->getValue());

  switch (retval.mId)
  {
  case csDATA:
	retval = mServices->getSpaceProvider()->getD(mData->getData());
    break;
  case csTOOL:
	retval = mServices->getSpaceProvider()->getT(mTools->getTool());
    break;
  case csSENSOR:
	retval = mServices->getSpaceProvider()->getT(mTools->getTool());
    break;
  default:
    retval.mRefObject = "";
    break;
  };

  return retval;
}
//------------------------------------------------------------------------------


}
