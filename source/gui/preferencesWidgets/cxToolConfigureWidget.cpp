#include <cxToolConfigureWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QDir>
#include <QLineEdit>
#include "cxEnumConverter.h"
#include "cxReporter.h"
#include "cxStateService.h"
#include "cxToolManager.h"
#include "cxSelectionGroupBox.h"
#include "cxToolListWidget.h"
#include "cxDataLocations.h"
#include "cxApplicationStateMachine.h"
#include "cxTrackerConfiguration.h"

namespace cx
{

ToolConfigureGroupBox::ToolConfigureGroupBox(QWidget* parent) :
    QGroupBox(parent),
    mClinicalApplication(mdCOUNT),
    mConfigFilesComboBox(new QComboBox()),
    mConfigFilePathLineEdit(new QLineEdit()),
    mConfigFileLineEdit(new QLineEdit()),
    mReferenceComboBox(new QComboBox())
{
  Q_PROPERTY("userEdited")

  mConfigFilesComboBox->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
  //mConfigFilesComboBox->setMinimumSize(200, 0);
  //mConfigFilesComboBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  mApplicationGroupBox = new SelectionGroupBox("Applications", stateService()->getApplication()->getAllApplicationNames(), Qt::Vertical, true, NULL);
  mApplicationGroupBox->setEnabledButtons(false); //< application application is determined by the application state chosen elsewhere in the system
  mApplicationGroupBox->hide(); // large and redundant box - info is only used for path generation, which can be found in the "Save Path" box
  mApplicationGroupBox->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
  mTrackingSystemGroupBox = new SelectionGroupBox("Tracking systems", toolManager()->getSupportedTrackingSystems(), Qt::Horizontal, true, NULL);
  mToolListWidget = new ConfigToolListWidget(NULL);

  this->setClinicalApplicationSlot(string2enum<CLINICAL_APPLICATION>(stateService()->getApplication()->getActiveStateName()));

  QGroupBox* toolGroupBox = new QGroupBox();
  toolGroupBox->setTitle("Tools");
  QVBoxLayout* toolLayout = new QVBoxLayout();
  toolGroupBox->setLayout(toolLayout);
  toolLayout->addWidget(mToolListWidget);

  QGridLayout* layout = new QGridLayout(this);
  int row=0;
  layout->addWidget(new QLabel("Selected config: "), row, 0, 1, 1);
  layout->addWidget(mConfigFilesComboBox, row, 1, 1, 1);
  row++;
  layout->addWidget(new QLabel("Save path: "), row, 0, 1, 1);
  layout->addWidget(mConfigFilePathLineEdit, row, 1, 1, 1);
  row++;
  layout->addWidget(new QLabel("File name: "), row, 0, 1, 1);
  layout->addWidget(mConfigFileLineEdit, row, 1, 1, 1);
  row++;
  layout->addWidget(mApplicationGroupBox, row, 0, 1, 2);
  row++;
  layout->addWidget(mTrackingSystemGroupBox, row, 0, 1, 2);
  row++;
  layout->addWidget(toolGroupBox, row, 0, 1, 2);
  layout->setRowStretch(row, 1);
  row++;
  layout->addWidget(new QLabel("Reference: "), row, 0, 1, 1);
  layout->addWidget(mReferenceComboBox, row, 1, 1, 1);

  //changes due to programming actions
  connect(mConfigFilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(configChangedSlot()));
  connect(mToolListWidget, SIGNAL(listSizeChanged()), this, SLOT(toolsChangedSlot()));
  connect(mTrackingSystemGroupBox, SIGNAL(selectionChanged()), this, SLOT(filterToolsSlot()));
  connect(mToolListWidget, SIGNAL(toolSelected(QString)), this, SIGNAL(toolSelected(QString)));

  //changes due to user actions
  connect(mConfigFilePathLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(pathEditedSlot()));
  connect(mConfigFilePathLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(configEditedSlot()));
  connect(mConfigFileLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(fileNameEditedSlot()));
  connect(mConfigFileLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(configEditedSlot()));
  connect(mConfigFileLineEdit, SIGNAL(editingFinished()), this, SLOT(filenameDoneEditingSlot()));
  connect(mApplicationGroupBox, SIGNAL(userClicked()), this, SLOT(configEditedSlot()));
  connect(mTrackingSystemGroupBox, SIGNAL(userClicked()), this, SLOT(configEditedSlot()));
  connect(mToolListWidget, SIGNAL(userChangedList()), this, SLOT(configEditedSlot()));
  connect(mReferenceComboBox, SIGNAL(activated(int)), this, SLOT(configEditedSlot()));

  this->populateConfigurations();
}

ToolConfigureGroupBox::~ToolConfigureGroupBox()
{}

void ToolConfigureGroupBox::setCurrentlySelectedCofiguration(QString configAbsoluteFilePath)
{
	QString cleanPath = QDir(configAbsoluteFilePath).absolutePath();
  int currentIndex = mConfigFilesComboBox->findData(cleanPath, Qt::ToolTipRole);
  if(currentIndex < 0)
  {
    currentIndex = 0;
    reportWarning("Tool configuration doesn't exist: " + cleanPath);
  }
  mConfigFilesComboBox->setCurrentIndex(currentIndex);
}

QString ToolConfigureGroupBox::getCurrenctlySelectedConfiguration() const
{
  QString retval;
  retval = mConfigFilesComboBox->itemData(mConfigFilesComboBox->currentIndex(), Qt::ToolTipRole).toString();
  return retval;
}

QString ToolConfigureGroupBox::requestSaveConfigurationSlot()
{
  QString retval;

  if(!mConfigFilesComboBox->itemData(mConfigFilesComboBox->currentIndex(), sEdited).toBool())
    return retval;

  // deconfigure toolmanager in order to be able to reread config data
  toolManager()->deconfigure();

  TrackerConfiguration::Configuration current = this->getCurrentConfiguration();
  TrackerConfigurationPtr config = toolManager()->getConfiguration();
  config->saveConfiguration(current);
//  ConfigurationFileParser::Configuration config = this->getCurrentConfiguration();
//  ConfigurationFileParser::saveConfiguration(config);

  retval = current.mFileName;

  this->populateConfigurations();

  return retval;
}

void ToolConfigureGroupBox::setClinicalApplicationSlot(CLINICAL_APPLICATION clinicalApplication)
{
  mClinicalApplication = clinicalApplication;
  this->setTitle("Tool configurations for "+enum2string(mClinicalApplication));

  this->populateConfigurations();
}

void ToolConfigureGroupBox::configChangedSlot()
{
	QStringList selectedApplications;
	QStringList selectedTrackingSystems;
	QStringList selectedTools;
	QString absoluteConfigFilePath = mConfigFilesComboBox->itemData(mConfigFilesComboBox->currentIndex(),
					Qt::ToolTipRole).toString();
	bool suggestDefaultNames;

	if (mConfigFilesComboBox->currentText().contains("<new config>"))
	{
		selectedApplications << enum2string(mClinicalApplication); // just want a default
		selectedTrackingSystems << enum2string(tsPOLARIS); //just want a default
		suggestDefaultNames = true;

		absoluteConfigFilePath = DataLocations::getRootConfigPath()
			+"/tool/"
			+enum2string(mClinicalApplication)
			+ "/";
	}
	else
	{
		TrackerConfigurationPtr config = toolManager()->getConfiguration();
		TrackerConfiguration::Configuration data = config->getConfiguration(absoluteConfigFilePath);

		selectedApplications << data.mClinicalApplication;
		selectedTrackingSystems << data.mTrackingSystem;
		selectedTools = data.mTools;

//		ConfigurationFileParser parser(absoluteConfigFilePath);

//		CLINICAL_APPLICATION application = parser.getApplicationapplication();
//		selectedApplications << enum2string(application);

//		std::vector<IgstkTracker::InternalStructure> trackers = parser.getTrackers();
//		for (unsigned i = 0; i < trackers.size(); ++i)
//		{
//			selectedTrackingSystems << enum2string(trackers[i].mType);
//		}

//		std::vector<QString> tools = parser.getAbsoluteToolFilePaths();
//		for (unsigned i = 0; i < tools.size(); ++i)
//		{
//			selectedTools << tools[i];
//		}
		suggestDefaultNames = false;
	}

//  std::cout << "absoluteConfigFilePath" << absoluteConfigFilePath << std::endl;
	QFile file(absoluteConfigFilePath);
	QFileInfo info(file);
	QString filePath = info.path();
	QString fileName = info.fileName();

	mConfigFilePathLineEdit->setText(filePath);
	this->setState(mConfigFilePathLineEdit, !suggestDefaultNames);

	mConfigFileLineEdit->setText(fileName);
	this->setState(mConfigFileLineEdit, !suggestDefaultNames);

	mApplicationGroupBox->setSelected(selectedApplications);
	mTrackingSystemGroupBox->setSelected(selectedTrackingSystems);
	mToolListWidget->configSlot(selectedTools);
}

void ToolConfigureGroupBox::configEditedSlot()
{
  this->setState(mConfigFilesComboBox, mConfigFilesComboBox->currentIndex(), true);
}

void ToolConfigureGroupBox::toolsChangedSlot()
{
  this->populateReference();
  this->filterToolsSlot();
}

void ToolConfigureGroupBox::filterToolsSlot()
{
  QStringList trackingsystemFilter = mTrackingSystemGroupBox->getSelected();
  mToolListWidget->filterSlot(trackingsystemFilter);
}

void ToolConfigureGroupBox::pathEditedSlot()
{
  this->setState(mConfigFilePathLineEdit, true);
}

void ToolConfigureGroupBox::fileNameEditedSlot()
{
  this->setState(mConfigFileLineEdit, true);
}

void ToolConfigureGroupBox::filenameDoneEditingSlot()
{
  if(mConfigFileLineEdit->text().contains(".xml", Qt::CaseInsensitive))
    mConfigFileLineEdit->setText(mConfigFileLineEdit->text().remove(".xml"));

  if(!mConfigFileLineEdit->text().endsWith(".xml", Qt::CaseInsensitive))
    mConfigFileLineEdit->setText(mConfigFileLineEdit->text()+".xml");
}

void ToolConfigureGroupBox::populateConfigurations()
{
  mConfigFilesComboBox->clear();

  QDir dir(DataLocations::getRootConfigPath()+"/tool/"+enum2string(mClinicalApplication));
  dir.setFilter(QDir::Files);

  QStringList nameFilters;
  nameFilters << "*.xml";
  dir.setNameFilters(nameFilters);

  QString newConfig("<new config>");
	this->addConfigurationToComboBox(newConfig, this->generateConfigName());

  QStringList configlist = dir.entryList();
  foreach(QString filename, configlist)
  {
    QFile file(dir.absolutePath()+"/"+filename);
    QFileInfo info(file);
		this->addConfigurationToComboBox(filename, info.absoluteFilePath());
  }

  int currentIndex = mConfigFilesComboBox->findText(newConfig);
  mConfigFilesComboBox->setCurrentIndex(currentIndex);
}

int ToolConfigureGroupBox::addConfigurationToComboBox(QString displayName, QString absoluteFilePath, bool edited)
{
  mConfigFilesComboBox->addItem(displayName);
  int index = mConfigFilesComboBox->findText(displayName);
  mConfigFilesComboBox->setItemData(index, absoluteFilePath, Qt::ToolTipRole);
  this->setState(mConfigFilesComboBox, index, false);

  return index;
}

void ToolConfigureGroupBox::setState(QComboBox* box, int index, bool edited)
{
  box->setItemData(index, edited, sEdited);
//  std::cout << "Config file " << box->itemText(index) << " now is set as " << (edited ? "" : "un") << "edited." << std::endl;

  if(edited && !mConfigFilePathLineEdit->property("userEdited").toBool() && !mConfigFileLineEdit->property("userEdited").toBool())
  {
//    std::cout << "Generating name..." << std::endl;
    QString absoluteConfigPaht = this->generateConfigName();
    QFile file(absoluteConfigPaht);
    QFileInfo info(file);
    QString filename = info.fileName();
    QString filepath = info.path();

    mConfigFilePathLineEdit->setText(filepath);
    mConfigFileLineEdit->setText(filename);
  }
}

TrackerConfiguration::Configuration ToolConfigureGroupBox::getCurrentConfiguration()
{
  TrackerConfiguration::Configuration retval;
  QString filename = mConfigFileLineEdit->text();
  QString filepath = mConfigFilePathLineEdit->text();
  retval.mFileName = filepath+"/"+filename;
//  retval.mClinical_app = string2enum<CLINICAL_APPLICATION>(mApplicationGroupBox->getSelected()[0]);
  retval.mClinicalApplication = mApplicationGroupBox->getSelected()[0];
  retval.mTrackingSystem = mTrackingSystemGroupBox->getSelected()[0];
  retval.mTools = mToolListWidget->getTools();
  retval.mReferenceTool = mReferenceComboBox->itemData(mReferenceComboBox->currentIndex(), Qt::ToolTipRole).toString();

//  QStringList selectedTools = mToolListWidget->getTools();
//  QString referencePath = mReferenceComboBox->itemData(mReferenceComboBox->currentIndex(), Qt::ToolTipRole).toString();

//  TRACKING_SYSTEM selectedTracker = string2enum<TRACKING_SYSTEM>(mTrackingSystemGroupBox->getSelected()[0]);

//  ConfigurationFileParser::ToolFilesAndReferenceVector toolfilesAndRefVector;
//  QFile configFile(retval.mFileName);
//  QFileInfo info(configFile);
//  QDir dir = info.dir();
//  foreach(QString absoluteToolPath, selectedTools)
//  {
//    QString relativeToolFilePath = dir.relativeFilePath(absoluteToolPath);
////    std::cout << "Relative tool file path: " << relativeToolFilePath << std::endl;

//    ConfigurationFileParser::ToolFileAndReference tool;
//    tool.first = relativeToolFilePath;

////    std::cout << "====" << std::endl;
////    std::cout << "absoluteToolPath " << absoluteToolPath << std::endl;
////    std::cout << "referencePath " << referencePath << std::endl;
//    tool.second = (absoluteToolPath == referencePath);
//    toolfilesAndRefVector.push_back(tool);
//  }

//  retval.mTrackersAndTools[selectedTracker] = toolfilesAndRefVector;

  return retval;
}

QString ToolConfigureGroupBox::generateConfigName()
{
  QString retval;

  QStringList applicationFilter = mApplicationGroupBox->getSelected();
  QStringList trackingsystemFilter = mTrackingSystemGroupBox->getSelected();
  QStringList absoluteToolFilePathsFilter = mToolListWidget->getTools();

  QString absoluteDirPath;
  QString trackingSystems;
  QString tools;

  absoluteDirPath = DataLocations::getRootConfigPath()+"/tool/"+((applicationFilter.size() >= 1) ? applicationFilter[0]+"/" : "")+""; //a config can only belong to one application

  foreach(QString string, trackingsystemFilter)
  {
    trackingSystems.append(string+"_");
  }

  foreach(QString string, absoluteToolFilePathsFilter)
  {
    QFile file(string);
    QFileInfo info(file);
    trackingSystems.append(info.baseName()+"_");
  }
  if(retval.endsWith("_", Qt::CaseInsensitive))
    retval.remove(retval.size()-2, 1);

  retval = absoluteDirPath+trackingSystems+tools+".xml";

  return retval;
}


void ToolConfigureGroupBox::setState(QLineEdit* line, bool userEdited)
{
//  std::cout << "line set to " << (userEdited ? "edited (should not generate new names from now)" : "not editet (generate from now)") << std::endl;
  QVariant value(userEdited);
  line->setProperty("userEdited", value);
}

void ToolConfigureGroupBox::populateReference()
{
	mReferenceComboBox->clear();

	int currentIndex = -1;

	TrackerConfigurationPtr config = toolManager()->getConfiguration();

	// populate list
	QStringList selectedTools = mToolListWidget->getTools();
	foreach(QString string, selectedTools)
	{
		if (config->getTool(string).mIsReference)
//		ToolFileParser parser(string);
//		IgstkTool::InternalStructure internal = parser.getTool();
//		if (internal.mIsReference)
		{
			currentIndex = this->addRefrenceToComboBox(string);
		}
	}

	// look for a selected reference
	QString configAbsoluteFilePath = mConfigFilesComboBox->itemData(mConfigFilesComboBox->currentIndex(), Qt::ToolTipRole).toString();
//	ConfigurationFileParser parser(configAbsoluteFilePath);
//	QString reference = parser.getAbsoluteReferenceFilePath();
	QString reference = config->getConfiguration(configAbsoluteFilePath).mReferenceTool;
	currentIndex = this->addRefrenceToComboBox(reference);

	// always select a reference if available:
	if (currentIndex<0)
		currentIndex = 0;

//	// if new: select the first one anyway
//	if (mConfigFilesComboBox->currentText().contains("<new config>") && (currentIndex==-1))
//		currentIndex = 0;

	mReferenceComboBox->setCurrentIndex(currentIndex);
}

int ToolConfigureGroupBox::addRefrenceToComboBox(QString absoluteRefereneFilePath)
{
	int index;

	QFile file(absoluteRefereneFilePath);
	QFileInfo info(file);
	QString refUid = info.dir().dirName();

	QStringList selectedTools = mToolListWidget->getTools();
	if (!selectedTools.count(absoluteRefereneFilePath))
		return -1;

	if (refUid.isEmpty())
		return -1;

	if (mReferenceComboBox->findText(refUid) < 0)
		mReferenceComboBox->addItem(refUid);
	index = mReferenceComboBox->findText(refUid);
	mReferenceComboBox->setItemData(index, info.absoluteFilePath(), Qt::ToolTipRole);

	return index;
}


}//namespace cx
