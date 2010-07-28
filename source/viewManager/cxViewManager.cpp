#include "cxViewManager.h"

#include <QGridLayout>
#include <QWidget>
#include <QTimer>
#include <QSettings>
#include <QTime>
#include <QAction>
#include <vtkRenderWindow.h>
#include <vtkImageData.h>
#include "sscProbeRep.h"
#include "sscVolumetricRep.h"
#include "sscMessageManager.h"
#include "sscXmlOptionItem.h"
#include "cxRepManager.h"
#include "cxView2D.h"
#include "cxView3D.h"
#include "cxViewGroup.h"
#include "cxViewWrapper.h"
#include "cxViewWrapper2D.h"
#include "cxViewWrapper3D.h"
#include "cxDataManager.h"
#include "cxToolManager.h"
#include "cxDataLocations.h"

namespace cx
{

ViewManager *ViewManager::mTheInstance = NULL;
ViewManager* viewManager() { return ViewManager::getInstance(); }
ViewManager* ViewManager::getInstance()
{
  if(mTheInstance == NULL)
   {
     mTheInstance = new ViewManager();
   }
   return mTheInstance;
}
void ViewManager::destroyInstance()
{}
ViewManager::ViewManager() :
  mLayout(new QGridLayout()),
  mMainWindowsCentralWidget(new QWidget()),
  MAX_3DVIEWS(2),
  MAX_2DVIEWS(15),
  mRenderingTimer(new QTimer(this)),
  mSettings(DataLocations::getSettings()),
  mRenderingTime(new QTime()),
  mNumberOfRenderings(0),
  mGlobal2DZoom(true),
  mGlobalObliqueOrientation(false)
{
  this->addDefaultLayouts();
  this->loadGlobalSettings();

  mLayout->setSpacing(2);
  mLayout->setMargin(4);
  mMainWindowsCentralWidget->setLayout(mLayout);

  mView3DNames.resize(MAX_3DVIEWS);
  for (unsigned i=0; i<mView3DNames.size(); ++i)
    mView3DNames[i] = "View3D_"+string_cast(i+1);

  mView2DNames.resize(MAX_2DVIEWS);
  for (unsigned i=0; i<mView2DNames.size(); ++i)
    mView2DNames[i] = "View2D_"+string_cast(i+1);

  for(int i=0; i<MAX_3DVIEWS; i++)
  {
    View3D* view = new View3D(mView3DNames[i], mView3DNames[i],
                              mMainWindowsCentralWidget);
    view->hide();
    mView3DMap[view->getUid()] = view;
    mViewMap[view->getUid()] = view;
    
    //Turn off rendering in vtkRenderWindowInteractor
    view->getRenderWindow()->GetInteractor()->EnableRenderOff();
  }
  for(int i=0; i<MAX_2DVIEWS; i++)
  {
    View2D* view = new View2D(mView2DNames[i], mView2DNames[i],
                              mMainWindowsCentralWidget);
    view->hide();
    mView2DMap[view->getUid()] = view;
    mViewMap[view->getUid()] = view;
    
    //Turn off rendering in vtkRenderWindowInteractor
    view->getRenderWindow()->GetInteractor()->EnableRenderOff();
  }

  // initialize view groups:
  ViewGroupPtr group;

  group.reset(new ViewGroup());
  group->addViewWrapper(ViewWrapper3DPtr(new ViewWrapper3D(1, mView3DMap["View3D_1"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_7"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_8"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_9"])));
  mViewGroups.push_back(group);

  group.reset(new ViewGroup());
  group->addViewWrapper(ViewWrapper3DPtr(new ViewWrapper3D(2, mView3DMap["View3D_2"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_10"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_11"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_12"])));
  mViewGroups.push_back(group);

  group.reset(new ViewGroup());
  //group->addViewWrapper(ViewWrapper3DPtr(new ViewWrapper3D(3, mView3DMap["View3D_3"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_13"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_14"])));
  group->addViewWrapper(ViewWrapper2DPtr(new ViewWrapper2D(mView2DMap["View2D_15"])));
  mViewGroups.push_back(group);

  this->syncOrientationMode(SyncedValue::create(0));

  // set start layout
  this->setActiveLayout("LAYOUT_3DACS_2X2");
  //this->setActiveLayout(LAYOUT_3DACS_1X3_SNW);

  mRenderingTimer->start(mSettings->value("renderingInterval").toInt());
  connect(mRenderingTimer, SIGNAL(timeout()),
          this, SLOT(renderAllViewsSlot()));
  
  mShadingOn = mSettings->value("shadingOn").toBool();

  mGlobalZoom2DVal = SyncedValue::create(1);
  this->setGlobal2DZoom(mGlobal2DZoom);

  mRenderingTime->start();
}
ViewManager::~ViewManager()
{}

void ViewManager::setRegistrationMode(ssc::REGISTRATION_STATUS mode)
{
  for (unsigned i=0; i<mViewGroups.size(); ++i)
    mViewGroups[i]->setRegistrationMode(mode);
}

QString ViewManager::getActiveLayout() const
{
  return mActiveLayout;
}

/**Change layout from current to layout.
 */
void ViewManager::setActiveLayout(const QString& layout)
{
  if (mActiveLayout==layout)
    return;

  deactivateCurrentLayout();
  activateLayout(layout);
}

ViewWrapperPtr ViewManager::getActiveView() const
{
  return mActiveView;
}

void ViewManager::setActiveView(ViewWrapperPtr viewWrapper)
{
  if(mActiveView && viewWrapper &&
      (mActiveView->getView()->getUid() == viewWrapper->getView()->getUid()))
    return;

  mActiveView = viewWrapper;
  emit activeViewChanged();
  ssc::messageManager()->sendInfo("Active view set to "+mActiveView->getView()->getUid());
}

void ViewManager::setActiveView(std::string viewUid)
{
  for(unsigned i=0; i<mViewGroups.size(); ++i)
  {
    ViewWrapperPtr viewWrapper = mViewGroups[i]->getViewWrapperFromViewUid(viewUid);
    if(viewWrapper)
    {
      this->setActiveView(viewWrapper);
      return;
    }
  }
}

void ViewManager::syncOrientationMode(SyncedValuePtr val)
{
  for(unsigned i=0; i<mViewGroups.size(); ++i)
  {
    mViewGroups[i]->syncOrientationMode(val);
  }
}


void ViewManager::setGlobal2DZoom(bool global)
{
  mGlobal2DZoom = global;

  for (unsigned i=0; i<mViewGroups.size(); ++i)
  {
    mViewGroups[i]->setGlobal2DZoom(mGlobal2DZoom, mGlobalZoom2DVal);
  }
}

bool ViewManager::getGlobal2DZoom()
{
  return mGlobal2DZoom;
}

void ViewManager::addXml(QDomNode& parentNode)
{
  QDomDocument doc = parentNode.ownerDocument();
  QDomElement viewManagerNode = doc.createElement("viewManager");
  parentNode.appendChild(viewManagerNode);

  QDomElement global2DZoomNode = doc.createElement("global2DZoom");
  global2DZoomNode.appendChild(doc.createTextNode(string_cast(mGlobal2DZoom).c_str()));
  viewManagerNode.appendChild(global2DZoomNode);

  QDomElement activeLayoutNode = doc.createElement("activeLayout");
  activeLayoutNode.appendChild(doc.createTextNode(mActiveLayout));
  viewManagerNode.appendChild(activeLayoutNode);

  QDomElement activeViewNode = doc.createElement("activeView");
  if(mActiveView)
    activeViewNode.appendChild(doc.createTextNode(mActiveView->getView()->getUid().c_str()));
  viewManagerNode.appendChild(activeLayoutNode);

  QDomElement viewGroupsNode = doc.createElement("viewGroups");
  viewManagerNode.appendChild(viewGroupsNode);
  for (unsigned i=0; i<mViewGroups.size(); ++i)
  {
    QDomElement viewGroupNode = doc.createElement("viewGroup");
    viewGroupNode.setAttribute("index", i);
    viewGroupsNode.appendChild(viewGroupNode);

    mViewGroups[i]->addXml(viewGroupNode);
  }

//  QDomElement layoutsNode = doc.createElement("layouts");
//  viewManagerNode.appendChild(layoutsNode);
//  for (LayoutDataVector::iterator iter=mLayouts.begin(); iter!=mLayouts.end(); ++iter)
//  {
//    if (!this->isCustomLayout(iter->getUid()))
//      continue; // dont store default layouts - they are created automatically.
//
//    QDomElement layoutNode = doc.createElement("layout");
//    layoutsNode.appendChild(layoutNode);
//    iter->addXml(layoutNode);
//  }


}

void ViewManager::parseXml(QDomNode viewmanagerNode)
{
//  mLayouts.clear();
//
//  QDomElement layouts = viewmanagerNode.namedItem("layouts").toElement();
//  QDomNode layout = layouts.firstChild();
//  for( ; !layout.isNull(); layout = layout.nextSibling())
//  {
//    if (layout.toElement().tagName()!="layout")
//      continue;
//
//    LayoutData data;
//    data.parseXml(layout);
//    this->setLayoutData(data);
////    mLayouts.push_back(data);
//  }
//  this->addDefaultLayouts(); // ensure we overwrite loaded layouts

  QString activeViewString;
  QDomNode child = viewmanagerNode.firstChild();
  while(!child.isNull())
  {
    if(child.toElement().tagName() == "global2DZoom")
    {
      const QString global2DZoomString = child.toElement().text();
      if(!global2DZoomString.isEmpty() && global2DZoomString.toInt() == 0)
        this->setGlobal2DZoom(false);
      else
        this->setGlobal2DZoom(true);
    }else if(child.toElement().tagName() == "activeLayout")
    {
      const QString activeLayoutString = child.toElement().text();
      if(!activeLayoutString.isEmpty())
        this->setActiveLayout(activeLayoutString);
    }
    else if(child.toElement().tagName() == "activeView")
    {
      activeViewString = child.toElement().text();
      //set active view after all viewgroups are properly set up
      //if(!activeViewString.isEmpty())
        //this->setActiveView(getViewWrapper(activeViewString.toStdString()));
    }
    child = child.nextSibling();
  }

  QDomElement viewgroups = viewmanagerNode.namedItem("viewGroups").toElement();
  QDomNode viewgroup = viewgroups.firstChild();
  while(!viewgroup.isNull())
  {
    if (viewgroup.toElement().tagName()!="viewGroup")
    {
      viewgroup = viewgroup.nextSibling();
      continue;
    }
    int index = viewgroup.toElement().attribute("index").toInt();

    if (index<0 || index>=int(mViewGroups.size()))
    {
      viewgroup = viewgroup.nextSibling();
      continue;
    }

    mViewGroups[index]->parseXml(viewgroup);

    viewgroup = viewgroup.nextSibling();
  }

  this->setActiveView(activeViewString.toStdString());
}

QWidget* ViewManager::stealCentralWidget()
{
  return mMainWindowsCentralWidget;
}

ViewManager::View2DMap* ViewManager::get2DViews()
{
  return &mView2DMap;
}

ViewManager::View3DMap* ViewManager::get3DViews()
{
  return &mView3DMap;
}

ssc::View* ViewManager::getView(const std::string& uid)
{
  ssc::View* view = NULL;
  View2DMap::iterator it2 = mView2DMap.find(uid);
  if(it2 != mView2DMap.end())
  {
    view = (*it2).second;
  }
  View3DMap::iterator it3 = mView3DMap.find(uid);
  if(it3 != mView3DMap.end())
  {
    view = (*it3).second;
  }
  return view;
}

View2D* ViewManager::get2DView(const std::string& uid)
{
  View2D* view = NULL;
  View2DMap::iterator it2 = mView2DMap.find(uid);
  if(it2 != mView2DMap.end())
  {
    view = (*it2).second;
  }
  return view;
}

View3D* ViewManager::get3DView(const std::string& uid)
{
  View3D* view = NULL;
  View3DMap::iterator it3 = mView3DMap.find(uid);
  if(it3 != mView3DMap.end())
  {
    view = (*it3).second;
  }
  return view;
}

/**deactivate the current layout, leaving an empty layout
 */
void ViewManager::deactivateCurrentLayout()
{
  for (ViewMap::iterator iter=mViewMap.begin(); iter!=mViewMap.end(); ++iter)
    deactivateView(iter->second);

  this->setStretchFactors(LayoutRegion(0, 0, 10, 10), 0);
}

/**activate a layout. Assumes the previous layout is already deactivated.
 */
void ViewManager::activateLayout(const QString& toType)
{
  LayoutData next = this->getLayoutData(toType);
  if (next.getUid().isEmpty())
    return;

  std::cout << streamXml2String(next) << std::endl;

  //TODO: the indexing into the view vector is both bad and evil. Constrains user and confuses developer. Undo!
  std::map<int,int> count3D;
  std::map<int,int> count2D;

  for (LayoutData::iterator iter=next.begin(); iter!=next.end(); ++iter)
  {
    LayoutData::ViewData view = *iter;

    if (view.mGroup<0 || view.mPlane==ssc::ptCOUNT)
      continue;

    if (!count2D.count(view.mGroup))
      count2D[view.mGroup] = 0;
    if (!count3D.count(view.mGroup))
      count3D[view.mGroup] = 0;

    if (view.mPlane == ssc::ptNOPLANE)
    {
      if (count3D[view.mGroup]>=1)
      {
        ssc::messageManager()->sendError("Adding >1 3D view per group not permitted"); // due to limitations in how views are hardcoded into groups
        continue;
      }
      activate3DView(view.mGroup, count3D[view.mGroup]++, view.mRegion);
    }
    else
    {
      if (count2D[view.mGroup]>=3)
      {
        ssc::messageManager()->sendError("Adding >3 2D views per group not permitted"); // due to limitations in how views are hardcoded into groups
        continue;
      }
      activate2DView(view.mGroup, 1 + count2D[view.mGroup]++, view.mPlane, view.mRegion);
    }
  }

  mActiveLayout = toType;
  emit activeLayoutChanged();

  ssc::messageManager()->sendInfo("Layout changed to "+ string_cast(this->getLayoutData(mActiveLayout).getName()));
}
  
void ViewManager::deleteImageSlot(ssc::ImagePtr image)
{
  for (unsigned i=0; i<mViewGroups.size(); ++i)
    mViewGroups[i]->removeImage(image);

  emit imageDeletedFromViews(image);
}

void ViewManager::renderingIntervalChangedSlot(int interval)
{
  mRenderingTimer->stop();
  mRenderingTimer->start(interval);
}

void ViewManager::shadingChangedSlot(bool shadingOn)
{
  mShadingOn = shadingOn;

  // currently disabled: shading is now a property in ssc::Image.
  // Remove this method???
  
//  ssc::VolumetricRepPtr volumetricRep
//  = RepManager::getInstance()->getVolumetricRep("VolumetricRep_1");
//  if(volumetricRep->getImage())
//  {
//    if(shadingOn)
//      volumetricRep->getVtkVolume()->GetProperty()->ShadeOn();
//    else
//      volumetricRep->getVtkVolume()->GetProperty()->ShadeOff();
//  }
}

/** Set the stretch factors of columns and rows in mLayout.
 */
void ViewManager::setStretchFactors( LayoutRegion region, int stretchFactor)
{
  // set stretch factors for the affected cols to 1 in order to get even distribution
  for (int i=region.pos.col; i<region.pos.col+region.span.col; ++i)
  {
	  mLayout->setColumnStretch(i,stretchFactor);
  }
  // set stretch factors for the affected rows to 1 in order to get even distribution
  for (int i=region.pos.row; i<region.pos.row+region.span.row; ++i)
  {
	  mLayout->setRowStretch(i,stretchFactor);
  }
}

void ViewManager::activate2DView(int group, int index, ssc::PLANE_TYPE plane, LayoutRegion region)
{
  mViewGroups[group]->initializeView(index, plane);
  ssc::View* view = mViewGroups[group]->getViews()[index];
  mLayout->addWidget(view, region.pos.row, region.pos.col, region.span.row, region.span.col );
  this->setStretchFactors( region, 1);

  view->show();
}
void ViewManager::activate3DView(int group, int index, LayoutRegion region)
{
  ssc::View* view = mViewGroups[group]->getViews()[index];
  mLayout->addWidget(view, region.pos.row, region.pos.col, region.span.row, region.span.col );
  this->setStretchFactors( region, 1);
  view->show();
}

void ViewManager::deactivateView(ssc::View* view)
{
  view->hide();
  mLayout->removeWidget(view);
}

void ViewManager::addDefaultLayout(LayoutData data)
{
  mDefaultLayouts.push_back(data.getUid());
  mLayouts.push_back(data);
}

/** insert the hardcoded layouts into mLayouts.
 *
 */
void ViewManager::addDefaultLayouts()
{
  mDefaultLayouts.clear();

  {
    LayoutData layout;
    layout.resetUid("LAYOUT_3D_1X1");
    layout.setName("3D 1x1");
    layout.resize(1,1);
    layout.setView(0, ssc::ptNOPLANE, LayoutRegion(0, 0));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_3DACS_2X2");
    layout.setName("3DACS 2x2");
    layout.resize(2,2);
    layout.setView(0, ssc::ptNOPLANE,  LayoutRegion(0, 0));
    layout.setView(0, ssc::ptAXIAL,    LayoutRegion(0, 1));
    layout.setView(0, ssc::ptCORONAL,  LayoutRegion(1, 0));
    layout.setView(0, ssc::ptSAGITTAL, LayoutRegion(1, 1));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_3DACS_1X3");
    layout.setName("3DACS 1x3");
    layout.resize(3,3);
    layout.setView(0, ssc::ptNOPLANE,  LayoutRegion(0, 0, 3, 2));
    layout.setView(0, ssc::ptAXIAL,    LayoutRegion(0, 2));
    layout.setView(0, ssc::ptCORONAL,  LayoutRegion(1, 2));
    layout.setView(0, ssc::ptSAGITTAL, LayoutRegion(2, 2));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_ACS_1X3");
    layout.setName("ACS 1x3");
    layout.resize(1,3);
    layout.setView(0, ssc::ptAXIAL,    LayoutRegion(0, 0));
    layout.setView(0, ssc::ptCORONAL,  LayoutRegion(0, 1));
    layout.setView(0, ssc::ptSAGITTAL, LayoutRegion(0, 2));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_ACSACS_2X3");
    layout.setName("ACSACS 2x3");
    layout.resize(2,3);
    layout.setView(0, ssc::ptAXIAL,    LayoutRegion(0, 0));
    layout.setView(0, ssc::ptCORONAL,  LayoutRegion(0, 1));
    layout.setView(0, ssc::ptSAGITTAL, LayoutRegion(0, 2));
    layout.setView(1, ssc::ptAXIAL,    LayoutRegion(1, 0));
    layout.setView(1, ssc::ptCORONAL,  LayoutRegion(1, 1));
    layout.setView(1, ssc::ptSAGITTAL, LayoutRegion(1, 2));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_Any_2X3");
    layout.setName("Any 2x3");
    layout.resize(2,3);
    layout.setView(0, ssc::ptANYPLANE,  LayoutRegion(0, 0));
    layout.setView(0, ssc::ptSIDEPLANE, LayoutRegion(1, 0));
    layout.setView(1, ssc::ptANYPLANE,  LayoutRegion(0, 1));
    layout.setView(1, ssc::ptSIDEPLANE, LayoutRegion(1, 1));
    layout.setView(2, ssc::ptANYPLANE,  LayoutRegion(0, 2));
    layout.setView(2, ssc::ptSIDEPLANE, LayoutRegion(1, 2));
    this->addDefaultLayout(layout);
  }
  {
    LayoutData layout;
    layout.resetUid("LAYOUT_3DAny_1X2");
    layout.setName("3DAny 1x2");
    layout.resize(1,2);
    layout.setView(0, ssc::ptNOPLANE,   LayoutRegion(0, 0));
    layout.setView(0, ssc::ptANYPLANE,  LayoutRegion(0, 1));
    this->addDefaultLayout(layout);
  }
}

void ViewManager::renderAllViewsSlot()
{
  for(ViewMap::iterator iter=mViewMap.begin(); iter != mViewMap.end(); ++iter)
  {
    if(iter->second->isVisible())
    {
      iter->second->getRenderWindow()->Render(); // previous version: renders even when nothing is changed
      //iter->second->render(); // render only changed scenegraph
    }
  }
  
  if(mRenderingTime->elapsed()>1000)
  {
    emit fps(mNumberOfRenderings);
    mRenderingTime->restart();
    mNumberOfRenderings = 1;
  }
  else
    mNumberOfRenderings++;
}

LayoutData ViewManager::getLayoutData(const QString uid) const
{
  unsigned pos = this->findLayoutData(uid);
  if (pos!=mLayouts.size())
    return mLayouts[pos];
  return LayoutData();
}

std::vector<QString> ViewManager::getAvailableLayouts() const
{
  std::vector<QString> retval;
  for (unsigned i=0; i<mLayouts.size(); ++i)
  {
    retval.push_back(mLayouts[i].getUid());
  }
  return retval;
}

void ViewManager::setLayoutData(const LayoutData& data)
{
  bool activeChange = mActiveLayout==data.getUid();
  unsigned pos = this->findLayoutData(data.getUid());
  if (pos==mLayouts.size())
    mLayouts.push_back(data);
  else
    mLayouts[pos] = data;

  if (activeChange)
  {
    mActiveLayout = "";
    this->setActiveLayout(data.getUid());
  }
  this->saveGlobalSettings();
  emit activeLayoutChanged();
}

QString ViewManager::generateLayoutUid() const
{
  int count = 0;

  for (LayoutDataVector::const_iterator iter=mLayouts.begin(); iter!=mLayouts.end(); ++iter)
  {
    if (iter->getUid() == qstring_cast(count))
      count = iter->getUid().toInt() + 1;
  }
  return qstring_cast(count);
}

void ViewManager::deleteLayoutData(const QString uid)
{
  mLayouts.erase(mLayouts.begin()+findLayoutData(uid));
  this->saveGlobalSettings();
  emit activeLayoutChanged();
}

unsigned ViewManager::findLayoutData(const QString uid) const
{
  for (unsigned i=0; i<mLayouts.size(); ++i)
  {
    if (mLayouts[i].getUid() == uid)
      return i;
  }
  return mLayouts.size();
}

QActionGroup* ViewManager::createLayoutActionGroup()
{
  QActionGroup* retval = new QActionGroup(NULL);
  retval->setExclusive(true);

  // add default layouts
  //std::vector<QString> layouts = this->getAvailableLayouts();
  for (unsigned i=0; i<mLayouts.size(); ++i)
  {
    if (!this->isCustomLayout(mLayouts[i].getUid()))
      this->addLayoutAction(mLayouts[i].getUid(), retval);
  }

  // add separator
  QAction* sep = new QAction(retval);
  sep->setSeparator(this);
  //retval->addAction(sep);

  // add custom layouts
  for (unsigned i=0; i<mLayouts.size(); ++i)
  {
    if (this->isCustomLayout(mLayouts[i].getUid()))
      this->addLayoutAction(mLayouts[i].getUid(), retval);
  }

  // set checked status
  QString type = this->getActiveLayout();
  QList<QAction*> actions = retval->actions();
  for (int i=0; i<actions.size(); ++i)
  {
    if (actions[i]->data().toString()==type)
      actions[i]->setChecked(true);
  }

  return retval;
}

/** Add one layout as an action to the layout menu.
 */
QAction* ViewManager::addLayoutAction(QString layout, QActionGroup* group)
{
  LayoutData data = this->getLayoutData(layout);
  QAction* action = new QAction(data.getName(), group);
  action->setCheckable(true);
  action->setData(QVariant(layout));
  connect(action, SIGNAL(triggered()), this, SLOT(setLayoutActionSlot()));
  return action;
}

/** Called when a layout is selected: introspect the sending action
 *  in order to get correct layout; set it.
 */
void ViewManager::setLayoutActionSlot()
{
  QAction* action = dynamic_cast<QAction*>(sender());
  if (!action)
    return;
  this->setActiveLayout(action->data().toString());
}

bool ViewManager::isCustomLayout(const QString& uid) const
{
  return !std::count(mDefaultLayouts.begin(), mDefaultLayouts.end(), uid);
}

void ViewManager::loadGlobalSettings()
{
  QDomDocument doc("CustusX");
  doc.appendChild(doc.createElement("root"));
  ssc::XmlOptionFile file(cx::DataLocations::getXmlSettingsFile(), doc);

  QDomElement viewmanagerNode = file.getElement("viewmanager");
  // load custom layouts:
  mLayouts.clear();

  QDomElement layouts = viewmanagerNode.namedItem("layouts").toElement();
  QDomNode layout = layouts.firstChild();
  for( ; !layout.isNull(); layout = layout.nextSibling())
  {
    if (layout.toElement().tagName()!="layout")
      continue;

    LayoutData data;
    data.parseXml(layout);
    this->setLayoutData(data);
//    mLayouts.push_back(data);
  }
  this->addDefaultLayouts(); // ensure we overwrite loaded layouts
}

void ViewManager::saveGlobalSettings()
{
  QDomDocument doc("CustusX");
  doc.appendChild(doc.createElement("root"));
  ssc::XmlOptionFile file(cx::DataLocations::getXmlSettingsFile(), doc);
  QDomElement viewmanagerNode = file.getElement("viewmanager");

  file.getElement("viewmanager", "layouts").clear();
  QDomElement layoutsNode = file.getElement("viewmanager", "layouts");
  //viewManagerNode.appendChild(layoutsNode);
  //file.clean(layoutsNode);
  for (LayoutDataVector::iterator iter=mLayouts.begin(); iter!=mLayouts.end(); ++iter)
  {
    if (!this->isCustomLayout(iter->getUid()))
      continue; // dont store default layouts - they are created automatically.

    QDomElement layoutNode = doc.createElement("layout");
    layoutsNode.appendChild(layoutNode);
    iter->addXml(layoutNode);
  }

  file.save();
}

}//namespace cx
