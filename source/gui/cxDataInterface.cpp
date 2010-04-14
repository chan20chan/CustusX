/*
 * cxDataInterface.cpp
 *
 *  Created on: Apr 13, 2010
 *      Author: christiana
 */
#include "cxDataInterface.h"
#include "sscImage.h"
#include "sscTypeConversions.h"
#include "cxDataManager.h"
#include "cxMessageManager.h"
#include "sscImageLUT2D.h"
#include "cxDataManager.h"

namespace cx
{

DoubleDataInterfaceActiveImageBase::DoubleDataInterfaceActiveImageBase()
{
  connect(dataManager(), SIGNAL(activeImageChanged(const std::string&)), this, SLOT(activeImageChanged()));
}
void DoubleDataInterfaceActiveImageBase::activeImageChanged()
{
  if (mImage)
    disconnect(mImage->getLookupTable2D().get(), SIGNAL(transferFunctionsChanged()), this, SIGNAL(changed()));

  mImage = dataManager()->getActiveImage();

  if (mImage)
    connect(mImage->getLookupTable2D().get(), SIGNAL(transferFunctionsChanged()), this, SIGNAL(changed()));

  emit changed();
}
double DoubleDataInterfaceActiveImageBase::getValue() const
{
  if (!mImage)
    return 0.0;
  return getValueInternal();
}
bool DoubleDataInterfaceActiveImageBase::setValue(double val)
{
  if (!mImage)
    return false;
  setValueInternal(val);
  return true;
}

//---------------------------------------------------------
//---------------------------------------------------------

double DoubleDataInterface2DWindow::getValueInternal() const
{
  return mImage->getLookupTable2D()->getWindow();
}
void DoubleDataInterface2DWindow::setValueInternal(double val)
{
  mImage->getLookupTable2D()->setWindow(val);
}
ssc::DoubleRange DoubleDataInterface2DWindow::getValueRange() const
{
  if (!mImage)
    return ssc::DoubleRange();
  double range = mImage->getRange();
  return ssc::DoubleRange(1,range,range/1000.0);
}

//---------------------------------------------------------
//---------------------------------------------------------


double DoubleDataInterface2DLevel::getValueInternal() const
{
  return mImage->getLookupTable2D()->getLevel();
}
void DoubleDataInterface2DLevel::setValueInternal(double val)
{
  mImage->getLookupTable2D()->setLevel(val);
}
ssc::DoubleRange DoubleDataInterface2DLevel::getValueRange() const
{
  if (!mImage)
    return ssc::DoubleRange();

  double max = mImage->getMax();
  return ssc::DoubleRange(1,max,max/1000.0);
}

//---------------------------------------------------------
//---------------------------------------------------------

SliderGroupWidget::SliderGroupWidget(QWidget* parent, ssc::DoubleDataInterfacePtr dataInterface, QGridLayout* gridLayout, int row)
{
  mData = dataInterface;
  connect(mData.get(), SIGNAL(changed()), this, SLOT(dataChanged()));

  QHBoxLayout* topLayout = new QHBoxLayout;
  topLayout->setMargin(0);
  this->setLayout(topLayout);

  mLabel = new QLabel(this);
  mLabel->setText(mData->getValueName());
  topLayout->addWidget(mLabel);

  mEdit = new ssc::DoubleLineEdit(this);
  topLayout->addWidget(mEdit);
  connect(mEdit, SIGNAL(textEdited(const QString&)), this, SLOT(textEditedSlot(const QString&)));

  mSlider = new ssc::DoubleSlider(this);
  mSlider->setOrientation(Qt::Horizontal);
  mSlider->setDoubleRange(mData->getValueRange());
  topLayout->addWidget(mSlider);
  connect(mSlider, SIGNAL(doubleValueChanged(double)), this, SLOT(doubleValueChanged(double)));


  if (gridLayout) // add to input gridlayout
  {
    gridLayout->addWidget(mLabel,  row, 0);
    gridLayout->addWidget(mEdit,   row, 1);
    gridLayout->addWidget(mSlider, row, 2);
  }
  else // add directly to this
  {
    topLayout->addWidget(mLabel);
    topLayout->addWidget(mEdit);
    topLayout->addWidget(mSlider);
  }

  dataChanged();
}

void SliderGroupWidget::doubleValueChanged(double val)
{
  if (ssc::similar(val, mData->getValue()))
      return;

  mData->setValue(val);
}

void SliderGroupWidget::textEditedSlot(const QString& text)
{
  double newVal = mEdit->getDoubleValue(mData->getValue());

  if (ssc::similar(newVal, mData->getValue()))
      return;

  mData->setValue(newVal);
}

void SliderGroupWidget::dataChanged()
{
  mSlider->setDoubleRange(mData->getValueRange()); // in case the image is changed

  ssc::DoubleRange range = mData->getValueRange();
  //std::cout << "SliderGroup::dataChanged() " << range.min() << "," << range.max() << "," << range.step() << std::endl;
  mSlider->setDoubleValue(mData->getValue());
  mEdit->setDoubleValue(mData->getValue());
}

} // namespace cx
