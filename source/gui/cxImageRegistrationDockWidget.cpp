#include "cxImageRegistrationDockWidget.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <vtkDoubleArray.h>
#include "sscDataManager.h"
#include "cxRepManager.h"
#include "cxViewManager.h"
#include "cxMessageManager.h"
#include "cxView3D.h"
/**
 * cxImageRegistrationDockWidget.cpp
 *
 * \brief
 *
 * \date Jan 27, 2009
 * \author: Janne Beate Bakeng, SINTEF
 */

namespace cx
{
ImageRegistrationDockWidget::ImageRegistrationDockWidget() :
  mGuiContainer(new QWidget(this)),
  mVerticalLayout(new QVBoxLayout(mGuiContainer)),
  mImagesComboBox(new QComboBox(mGuiContainer)),
  mLandmarkTableWidget(new QTableWidget(mGuiContainer)),
  mAddPointButton(new QPushButton("Add point", mGuiContainer)),
  mRemovePointButton(new QPushButton("Remove point", mGuiContainer)),
  mRepManager(RepManager::getInstance()),
  mDataManager(DataManager::getInstance()),
  mViewManager(ViewManager::getInstance()),
  mMessageManager(MessageManager::getInstance()),
  mCurrentRow(-1),
  mCurrentColumn(-1)
{
  //dock widget
  this->setWindowTitle("Image Registration");
  this->setWidget(mGuiContainer);
  connect(this, SIGNAL(visibilityChanged(bool)),
          this, SLOT(visibilityOfDockWidgetChangedSlot(bool)));

  //combobox
  mImagesComboBox->setEditable(false);
  connect(mImagesComboBox, SIGNAL(currentIndexChanged(const QString&)),
          this, SLOT(imageSelectedSlot(const QString&)));

  //pushbuttons
  mAddPointButton->setDisabled(true);
  connect(mAddPointButton, SIGNAL(clicked()),
          this, SLOT(addPointButtonClickedSlot()));
  mRemovePointButton->setDisabled(true);
  connect(mRemovePointButton, SIGNAL(clicked()),
          this, SLOT(removePointButtonClickedSlot()));

  //table widget
  connect(mLandmarkTableWidget, SIGNAL(cellClicked(int, int)),
          this, SLOT(landmarkSelectedSlot(int, int)));

  //layout
  mVerticalLayout->addWidget(mImagesComboBox);
  mVerticalLayout->addWidget(mLandmarkTableWidget);
  mVerticalLayout->addWidget(mAddPointButton);
  mVerticalLayout->addWidget(mRemovePointButton);

  mGuiContainer->setLayout(mVerticalLayout);
}
ImageRegistrationDockWidget::~ImageRegistrationDockWidget()
{}
void ImageRegistrationDockWidget::addPointButtonClickedSlot()
{
  VolumetricRepPtr volumetricRep = mRepManager->getVolumetricRep("VolumetricRep_1");
  if(volumetricRep.get() == NULL)
  {
    mMessageManager.sendError("Could not find a rep to add the landmark to.");
    return;
  }
  volumetricRep->makePointPermanent();
}
void ImageRegistrationDockWidget::removePointButtonClickedSlot()
{
  if(mCurrentRow < 0 || mCurrentColumn < 0)
    return;

  LandmarkRepPtr landmarkRep = mRepManager->getLandmarkRep("LandmarkRep_1");
  landmarkRep->requestRemovePermanentPoint(0,0,0); //TODO: implement
  //TODO:...or make sure landmarks are not sorted in image...
  //landmarkRep->removePermanentPoint(mCurrentRow+1);
}
void ImageRegistrationDockWidget::imageSelectedSlot(const QString& comboBoxText)
{
  std::string imageId = comboBoxText.toStdString();

  //find the image
  ssc::ImagePtr image = mDataManager->getImage(imageId);
  if(image.get() == NULL)
  {
    mMessageManager.sendError("Could not find the selected image in the DataManager.");
    return;
  }
  if(mCurrentImage)
  {
    //disconnect from the old image
    disconnect(mCurrentImage.get(), SIGNAL(landmarkAdded(double,double,double)),
              this, SLOT(imageLandmarksUpdateSlot(double,double,double)));
    disconnect(mCurrentImage.get(), SIGNAL(landmarkRemoved(double,double,double)),
              this, SLOT(imageLandmarksUpdateSlot(double,double,double)));
  }

  //Set new current image
  mCurrentImage = image;
  connect(mCurrentImage.get(), SIGNAL(landmarkAdded(double,double,double)),
          this, SLOT(imageLandmarksUpdateSlot(double,double,double)));
  connect(mCurrentImage.get(), SIGNAL(landmarkRemoved(double,double,double)),
          this, SLOT(imageLandmarksUpdateSlot(double,double,double)));

  //get the images landmarks and populate the landmark table
  this->populateTheLandmarkTableWidget(mCurrentImage);

  //TODO
  //show volumetric rep in View3D and InriaRep in View2D (linked)
    //view3D->getVolumetricRep->setImage()???
    //view3D->getInria3DRep->setImage()???
    //view2D->getInria2DRep->setImage()??? sync with the three others (2d, 2d and 3d)
  //show landmark reps
  View3D* view3D_1 = mViewManager->get3DView("View3D_1");
  VolumetricRepPtr volumetricRep = mRepManager->getVolumetricRep("VolumetricRep_1");
  LandmarkRepPtr landmarkRep = mRepManager->getLandmarkRep("LandmarkRep_1");
  volumetricRep->setImage(mCurrentImage);
  landmarkRep->setImage(mCurrentImage);
  view3D_1->setRep(volumetricRep);
  view3D_1->addRep(landmarkRep);
}
void ImageRegistrationDockWidget::visibilityOfDockWidgetChangedSlot(bool visible)
{
  if(visible)
    this->populateTheImageComboBox();
}
void ImageRegistrationDockWidget::imageLandmarksUpdateSlot(double notUsedX, double notUsedY, double notUsedZ)
{
  this->populateTheLandmarkTableWidget(mCurrentImage);
}
void ImageRegistrationDockWidget::populateTheImageComboBox()
{
  mImagesComboBox->clear();

  //get a list of images from the datamanager
  std::map<std::string, ssc::ImagePtr> images = mDataManager->getImages();
  if(images.size() == 0)
  {
    mAddPointButton->setDisabled(true);
    return;
  }

  //add these to the combobox
  typedef std::map<std::string, ssc::ImagePtr>::iterator iterator;
  int listPosition = 1;
  for(iterator i = images.begin(); i != images.end(); ++i)
  {
    mImagesComboBox->insertItem(listPosition, QString(i->first.c_str()));
    listPosition++;
  }
  //enable the add point button if any images was found
  mAddPointButton->setDisabled(false);
}
void ImageRegistrationDockWidget::landmarkSelectedSlot(int row, int column)
{
  mCurrentRow = row;
  mCurrentColumn = column;
}
void ImageRegistrationDockWidget::populateTheLandmarkTableWidget(ssc::ImagePtr image)
{
  vtkDoubleArrayPtr landmarks =  image->getLandmarks();
  int numberOfLandmarks = landmarks->GetNumberOfTuples();

  mLandmarkTableWidget->clear();
  mLandmarkTableWidget->setRowCount(numberOfLandmarks);
  mLandmarkTableWidget->setColumnCount(1);
  QStringList headerItems(QStringList() << "Image space");
  mLandmarkTableWidget->setHorizontalHeaderLabels(headerItems);
  mLandmarkTableWidget->horizontalHeader()->
    setResizeMode(QHeaderView::ResizeToContents);

  for(int row=1; row<=numberOfLandmarks; row++)
  {
    double* point = landmarks->GetTuple(row);

    QTableWidgetItem* columnOne = new QTableWidgetItem(tr("(%1, %2, %3)").arg(point[0]).arg(point[1]).arg(point[2]));
    columnOne->setFlags(Qt::ItemIsSelectable);

    mLandmarkTableWidget->setItem(row-1, 0, columnOne);
  }

  if(numberOfLandmarks == 0)
    mRemovePointButton->setDisabled(true);
  else
    mRemovePointButton->setDisabled(false);
}
}//namespace cx
