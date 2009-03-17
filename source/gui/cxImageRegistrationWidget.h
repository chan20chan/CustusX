#ifndef CXIMAGEREGISTRATIONWIDGET_H_
#define CXIMAGEREGISTRATIONWIDGET_H_

#include <map.h>
#include <QWidget>
#include "cxVolumetricRep.h"

/**
 * cxImageRegistrationDockWidget.h
 *
 * \brief Widget used as a tab in teh ContextDockWidget for image registration.
 *
 * \date Jan 27, 2009
 * \author: Janne Beate Bakeng, SINTEF
 */

class QVBoxLayout;
class QComboBox;
class QTableWidget;
class QPushButton;

namespace ssc
{
class DataManager;
}
namespace cx
{
class RepManager;
class ViewManager;
class RegistrationManager;
class MessageManager;

class ImageRegistrationWidget : public QWidget
{
  typedef ssc::DataManager DataManager;

  Q_OBJECT

public:
  ImageRegistrationWidget(); ///< sets up layout and connects signals and slots
  ~ImageRegistrationWidget(); ///< empty

protected slots:
  void currentImageChangedSlot(ssc::ImagePtr currentImage); ///< listens to the contextdockwidget for when the current image is changed
  void addLandmarkButtonClickedSlot(); ///< reacts when the Add Landmark button is clicked
  void editLandmarkButtonClickedSlot(); ///< reacts when the Edit Landmark button is clicked
  void removeLandmarkButtonClickedSlot(); ///< reacts when the Remove Landmark button is clicked
  //void imageSelectedSlot(const QString& comboBoxText); //TODO REMOVE
  //void visibilityOfDockWidgetChangedSlot(bool visible); //TODO REMOVE
  void imageLandmarksUpdateSlot(double, double, double,unsigned int); ///< updates the table widget when landmarks are added/edited or removed
  void landmarkSelectedSlot(int row, int column); ///<
  //void populateTheImageComboBox(); //TODO REMOVE
  void cellChangedSlot(int row,int column); ///< reacts when the user types in a (landmark) name


protected:
  void populateTheLandmarkTableWidget(ssc::ImagePtr image); ///< populates the table widget

  //gui
  QVBoxLayout* mVerticalLayout; ///< vertical layout is used
  QTableWidget* mLandmarkTableWidget; ///< the table widget presenting the landmarks
  QPushButton* mAddLandmarkButton; ///< the Add Landmark button
  QPushButton* mEditLandmarkButton; ///< the Edit Landmark button
  QPushButton* mRemoveLandmarkButton; ///< the Remove Landmark button

  //manageres
  RepManager* mRepManager; ///< has a pool of reps
  DataManager* mDataManager; ///< has all the data loaded into the system
  ViewManager* mViewManager; ///< controls layout of views and has a pool of views
  RegistrationManager* mRegistrationManager; ///< handles image and patient registration
  MessageManager* mMessageManager; ///< takes messages intended for the user

  //data
  ssc::ImagePtr mCurrentImage; ///< the image currently used in image registration
  std::map<int, bool> mLandmarkActiveMap; ///< mapping which landmarks are active (is going to be used when calculating the matrix)
  int mCurrentRow, mCurrentColumn; ///< which row and column are currently the choose ones
};
}//namespace cx

#endif /* CXIMAGEREGISTRATIONWIDGET_H_ */
