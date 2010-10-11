/*
 * cxToolWidget.h
 *
 *  Created on: Apr 22, 2010
 *      Author: christiana
 */
#ifndef CXTOOLPROPERTIESWIDGET_H_
#define CXTOOLPROPERTIESWIDGET_H_

#include <vector>
//#include <QtGui>
#include "sscForwardDeclarations.h"
#include "sscDoubleWidgets.h"
#include "sscStringDataAdapter.h"

class QCheckBox;

class UsConfigGui;

namespace cx
{

/** Adapter that connects to the current active tool.
 */
class ActiveToolStringDataAdapter : public ssc::StringDataAdapter
{
  Q_OBJECT
public:
  static ssc::StringDataAdapterPtr New() { return ssc::StringDataAdapterPtr(new ActiveToolStringDataAdapter()); }
  ActiveToolStringDataAdapter();
  virtual ~ActiveToolStringDataAdapter() {}
  
public: // basic methods
  virtual QString getValueName() const;
  virtual bool setValue(const QString& value);
  virtual QString getValue() const;
  
public: // optional methods
  virtual QString getHelp() const;
  virtual QStringList getValueRange() const;
  virtual QString convertInternal2Display(QString internal);
};
/** Widget that contains a select active tool combo box.
 */
class ActiveToolWidget : public QWidget
{
  Q_OBJECT
public:
  ActiveToolWidget(QWidget* parent);
~ActiveToolWidget() {}
};
  
  
/**
 * \class ToolPropertiesWidget
 *
 * \date 2010.04.22
 * \author: Christian Askeland, SINTEF
 */
class ToolPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  ToolPropertiesWidget(QWidget* parent);
  virtual ~ToolPropertiesWidget();

signals:

protected slots:
  void updateSlot();
  void dominantToolChangedSlot();
  void referenceToolChangedSlot();
  void configurationChangedSlot(int index);
  void toolsSectorConfigurationChangedSlot();///< Update the combo box when the tools configuration is changed outside the widget. Also used initially to read the tools value.

protected:
  virtual void showEvent(QShowEvent* event); ///<updates internal info before showing the widget
  virtual void hideEvent(QCloseEvent* event); ///<disconnects stuff

private:
  ToolPropertiesWidget();
  void populateUSSectorConfigBox();

  ssc::ToolPtr mReferenceTool;
  ssc::ToolPtr mActiveTool;

  QVBoxLayout* mToptopLayout;

  ssc::SliderGroupWidget* mToolOffsetWidget;
  QLabel* mActiveToolVisibleLabel;
  QLabel* mToolNameLabel;
  QLabel* mReferenceStatusLabel;
  QLabel* mTrackingSystemStatusLabel;
  
  QLabel* mUSSectorConfigLabel;   ///< Label for the mUSSectorConfigBox
  QComboBox* mUSSectorConfigBox;  ///< List of US sector config parameters: depth (and width)
};

}//end namespace cx


#endif /* CXTOOLPROPERTIESWIDGET_H_ */
