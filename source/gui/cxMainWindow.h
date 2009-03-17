#ifndef CXMAINWINDOW_H_
#define CXMAINWINDOW_H_

#include <QMainWindow>

/**
 * cxMainWindow.h
 *
 * \brief This is the main gui class which controls the workflow.
 *
 * \date Jan 20, 2009
 * \author: Janne Beate Bakeng, SINTEF
 */

class QAction;
class QMenu;
class QActionGroup;

namespace ssc
{
class DataManager;
}

namespace cx
{
typedef ssc::DataManager DataManager;

class RepManager;
class ViewManager;
class ToolManager;
class MessageManager;
class CustomStatusBar;
class ContextDockWidget;
class ImageRegistrationWidget;
class PatientRegistrationWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(); ///< sets up the initial gui
  ~MainWindow(); ///< empty

protected slots:
  //application menu
  void aboutSlot(); ///< TODO
  void preferencesSlot(); ///< TODO
  void quitSlot(); ///< TODO

  //workflow menu
  void patientDataWorkflowSlot(); ///< change state to patient data
  void imageRegistrationWorkflowSlot(); ///< change state to image registration
  void patientRegistrationWorkflowSlot(); ///< change state to patient registration
  void navigationWorkflowSlot(); ///< change state to navigation
  void usAcquisitionWorkflowSlot(); ///< change state to ultrasound acquisition

  //data menu
  void loadDataSlot(); ///< loads data into the datamanager

  //tool menu
  void configureSlot(); ///< lets the user choose which configuration files to use for the navigation

  //TODO: REMOVE
  //degugging
  void printSlot(std::string message); ///< prints messages from the toolmanager to std::cout

protected:
  enum WorkflowState
  {
    PATIENT_DATA,
    IMAGE_REGISTRATION,
    PATIENT_REGISTRATION,
    NAVIGATION,
    US_ACQUISITION
  }; ///< the different workflow(-states) the user can choose from

  void createActions(); ///< creates and connects (gui-)actions
  void createMenus(); ///< creates and add (gui-)menues
  void createToolBars(); ///< creates and adds toolbars for convenience
  void createStatusBar();  ///< //TODO

  //Takes care of removing and adding widgets depending on which workflow state the system is in
  void changeState(WorkflowState fromState, WorkflowState toState); ///< used to change state
  void activatePatientDataState(); ///< Should only be used by changeState(...)!
  void deactivatePatientDataState(); ///< Should only be used by changeState(...)!
  void activateImageRegistationState(); ///< Should only be used by changeState(...)!
  void deactivateImageRegistationState(); ///< Should only be used by changeState(...)!
  void activatePatientRegistrationState(); ///< Should only be used by changeState(...)!
  void deactivatePatientRegistrationState(); ///< Should only be used by changeState(...)!
  void activateNavigationState(); ///< Should only be used by changeState(...)!
  void deactivateNavigationState(); ///< Should only be used by changeState(...)!
  void activateUSAcquisitionState(); ///< Should only be used by changeState(...)!
  void deactivateUSAcquisitionState(); ///< Should only be used by changeState(...)!

  WorkflowState mCurrentWorkflowState; ///< the current workflow in use

  //managers
  ViewManager* mViewManager; ///< controls layout of views and has a pool of views
  DataManager* mDataManager; ///< has all the data loaded into the system
  ToolManager* mToolManager; ///< interface to the navigation system
  RepManager* mRepManager; ///< has a pool of reps
  MessageManager* mMessageManager; ///< takes messages intended for the user

  //gui
  QWidget* mCentralWidget; ///< central widget used for views

  //menus
  QMenu* mWorkflowMenu; ///< menu for choosing workflow
  QMenu* mDataMenu; ///< menu for loading data
  QMenu* mToolMenu; ///< menu for interacting with the navigation system
  QMenu* mLayoutMenu; ///< menu for changing view layouts

  //actions and actiongroups
  QAction* mPatientDataWorkflowAction; ///< action for switching to the patient data workflow
  QAction* mImageRegistrationWorkflowAction; ///< action for switching to the image registration workflow
  QAction* mPatientRegistrationWorkflowAction; ///< action for switching to the patient registraiton workflow
  QAction* mNavigationWorkflowAction; ///< action for switching to the navigation workflow
  QAction* mUSAcquisitionWorkflowAction; ///< action for switching to the ultrasound acqusition workflow
  QActionGroup* mWorkflowActionGroup; ///< grouping the workflow actions

  QAction* mLoadDataAction; ///< action for loading data into the datamanager

  QAction* mConfigureToolsAction; ///< action for configuring the toolmanager
  QAction* mInitializeToolsAction; ///< action for initializing contact with the navigation system
  QAction* mStartTrackingToolsAction; ///< action for asking the navigation system to start tracking
  QAction* mStopTrackingToolsAction; ///< action for asking the navigation system to stop tracking
  QActionGroup* mToolsActionGroup; ///< grouping the actions for contacting the navigation system

  QAction* m3D_1x1_LayoutAction; ///< action for switching to 3D_1x1 view layout
  QAction* m3DACS_2x2_LayoutAction; ///< action for switching to 3DACS_2x2 view layout
  QAction* m3DACS_1x3_LayoutAction; ///< action for switching to 3DACS_1x3 view layout
  QAction* mACSACS_2x3_LayoutAction; ///< action for switching to ACSACS_2x3 view layout
  QActionGroup* mLayoutActionGroup; ///< grouping the view layout actions

  //toolbars
  QToolBar* mDataToolBar; ///< toolbar for data actions
  QToolBar* mToolToolBar; ///< toolbar for navigation actions

  ContextDockWidget* mContextDockWidget; ///< dock widget for context sensitive widgets
  ImageRegistrationWidget* mImageRegistrationWidget; ///< interface for image registration
  PatientRegistrationWidget* mPatientRegistrationWidget; ///< interface for patient registration
  //cxVolumetricTFsDockWidget* mVolumetricTFsDockWidget; //TODO
  //CustomStatusBar* mCustomStatusBar; //TODO, not working yet
  int mImageRegistrationIndex, mPatientRegistrationIndex; ///< tab index for removing tabs is ContextDockWidget

  //Preferences
  QString mCurrentPatientDataFolder; ///< folder in which patient data is located
  QString mCurrentToolConfigFile; ///< file determining the navigation systems setup

};
}//namespace cx

#endif /* CXMAINWINDOW_H_ */
