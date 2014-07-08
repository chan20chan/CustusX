// This file is part of CustusX, an Image Guided Therapy Application.
//
// Copyright (C) 2008- SINTEF Technology & Society, Medical Technology
//
// CustusX is fully owned by SINTEF Medical Technology (SMT). CustusX source
// code and binaries can only be used by SMT and those with explicit permission
// from SMT. CustusX shall not be distributed to anyone else.
//
// CustusX is a research tool. It is NOT intended for use or certified for use
// in a normal clinical setting. SMT does not take responsibility for its use
// in any way.
//
// See CustusX_License.txt for more information.

#include "cxPluginFramework.h"

#include <QApplication>
#include <QStringList>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>

#include "ctkPluginFrameworkFactory.h"
#include "ctkPluginFramework.h"
#include "ctkPluginContext.h"
#include "ctkPluginException.h"

#include <ctkConfig.h>

#include "cxSettings.h"
#include "cxDataLocations.h"
#include "cxPluginFrameworkUtilities.h"
#include "cxReporter.h"
#include "cxFileHelpers.h"

#include <iostream>
#include "cxTypeConversions.h"

namespace cx
{

PluginFrameworkManager::PluginFrameworkManager()
{
	mSettingsBase = "pluginFramework";
	mSettingsSearchPaths = mSettingsBase + "/searchPaths";

	ctkProperties fwProps;
	QString storagePath = DataLocations::getSettingsPath() + "/pluginFramework";

	// remove settings as stored by CTK, because full paths are stored here, causing
	// problems when running both debug and release on the same machine (and similar).
	//removeNonemptyDirRecursively(storagePath);

	fwProps[ctkPluginConstants::FRAMEWORK_STORAGE] = storagePath;

	// clear settings stored by ctk
	fwProps[ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN] = ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT;

	mFrameworkFactory.reset(new ctkPluginFrameworkFactory(fwProps));
	mPluginLibFilter << "*.dll" << "*.so" << "*.dylib";
	this->loadState();
}

PluginFrameworkManager::~PluginFrameworkManager()
{
	this->saveState();
}

QString PluginFrameworkManager::convertToRelativePath(QString path) const
{
	QDir base = qApp->applicationDirPath();
	return base.relativeFilePath(path);
}

QString PluginFrameworkManager::convertToAbsolutePath(QString path) const
{
	if (QDir(path).isAbsolute())
		return QDir(path).absolutePath();

	QDir base = qApp->applicationDirPath();
	return QDir(base.path() + "/" + path).absolutePath();
}

void PluginFrameworkManager::loadState()
{
	QStringList paths = settings()->value(mSettingsSearchPaths, QStringList()).toStringList();
	this->setSearchPaths(paths);

	QStringList names = this->getPluginSymbolicNames();
	for (unsigned i=0; i<names.size(); ++i)
	{
		QString name = names[i];
		QString storedState = settings()->value(mSettingsBase+"/"+name).toString();
		this->loadPluginFromStoredState(name, storedState);
	}
}

void PluginFrameworkManager::loadPluginFromStoredState(QString symbolicName, QString storedState)
{	
	if (storedState.isEmpty())
	{
		report(QString("Initializing new plugin, autostarting: %1").arg(symbolicName));
		storedState = getStringForctkPluginState(ctkPlugin::ACTIVE);
	}
	else
	{
		report(QString("Initializing plugin to state [%2]: %1").arg(symbolicName).arg(storedState));
	}

	ctkPlugin::State state = getctkPluginStateForString(storedState);

	if (state==ctkPlugin::UNINSTALLED)
		return;

	if (state==ctkPlugin::ACTIVE)
		this->start(symbolicName, ctkPlugin::START_TRANSIENT);
	else
		this->install(symbolicName);
}

void PluginFrameworkManager::saveState()
{
	QStringList relativePaths;
	for (int i=0; i<mPluginSearchPaths.size(); ++i)
		relativePaths << this->convertToRelativePath(mPluginSearchPaths[i]);
	settings()->setValue(mSettingsSearchPaths, relativePaths);

	QStringList names = this->getPluginSymbolicNames();
	for (unsigned i=0; i<names.size(); ++i)
	{
		QString name = names[i];
		ctkPlugin::State state = this->getStateFromSymbolicName(name);
		settings()->setValue(mSettingsBase+"/"+name, getStringForctkPluginState(state));
	}
}

ctkPlugin::State PluginFrameworkManager::getStateFromSymbolicName(QString name)
{
	ctkPlugin::State state = ctkPlugin::UNINSTALLED;
	QSharedPointer<ctkPlugin> plugin = this->getInstalledPluginFromSymbolicName(name);
	if (plugin)
		state = plugin->getState();
	return state;
}

void PluginFrameworkManager::setSearchPaths(const QStringList& searchPath)
{
	mPluginSearchPaths.clear();

	for (int i=0; i<searchPath.size(); ++i)
		mPluginSearchPaths <<  this->convertToAbsolutePath(searchPath[i]);

	QStringList defPaths = DataLocations::getDefaultPluginsPath();
	for (unsigned i=0; i<defPaths.size(); ++i)
	{
		QString defPath = this->convertToAbsolutePath(defPaths[i]);
		if (!mPluginSearchPaths.count(defPath))
			mPluginSearchPaths << defPath;
	}

	mPluginSearchPaths.removeDuplicates();

	for (int i=0; i<searchPath.size(); ++i)
	{
		QApplication::addLibraryPath(searchPath[i]);
	}
	emit pluginPoolChanged();
}


QStringList PluginFrameworkManager::getSearchPaths() const
{
	return mPluginSearchPaths;
}

ctkPluginContext* PluginFrameworkManager::getPluginContext()
{
	return mFramework->getPluginContext();
}

QSharedPointer<ctkPluginFramework> PluginFrameworkManager::getPluginFramework()
{
	return mFramework;
}

void PluginFrameworkManager::initializeFramework()
{
	if (this->frameworkInitialized())
		return;

	QSharedPointer<ctkPluginFramework> framework = mFrameworkFactory->getFramework();

	try
	{
		framework->init();
	} catch (const ctkPluginException& exc)
	{
		qCritical() << "Failed to initialize the plug-in framework:" << exc;
	}
	mFramework = framework;
}

bool PluginFrameworkManager::frameworkInitialized() const
{
	return mFramework != 0;
}

bool PluginFrameworkManager::frameworkStarted() const
{
	return mFramework && (mFramework->getState() == ctkPlugin::ACTIVE);
}

void PluginFrameworkManager::startFramework()
{
	if (!this->frameworkInitialized())
		this->initializeFramework();

	if (this->frameworkStarted())
		return;

	try
	{
		mFramework->start();
	}
	catch (const ctkPluginException& exc)
	{
		qCritical() << "Failed to start the plug-in framework:" << exc;
	}
}

void PluginFrameworkManager::install(const QString& symbolicName)
{

	this->initializeFramework();
	if (!this->frameworkInitialized())
		return;

	QString pluginPath = this->getPluginPath(symbolicName);
	if (pluginPath.isEmpty())
		return;

	try
	{
		ctkPluginContext* pc = this->getPluginContext();
		pc->installPlugin(QUrl::fromLocalFile(pluginPath))->getPluginId();
	}
	catch (const ctkPluginException& exc)
	{
		qWarning() << "Failed to install plugin:" << symbolicName << ", " << exc;
	}
}

bool PluginFrameworkManager::start()
{
	this->startFramework();
	return this->frameworkStarted();
}

bool PluginFrameworkManager::stop()
{
	ctkPluginContext* pc = this->getPluginContext();
	// stop the framework
	QSharedPointer<ctkPluginFramework> fw = qSharedPointerCast<ctkPluginFramework>(pc->getPlugin(0));
	try
	{
		fw->stop();
		ctkPluginFrameworkEvent fe = fw->waitForStop(5000);
		if (fe.getType() == ctkPluginFrameworkEvent::FRAMEWORK_WAIT_TIMEDOUT)
		{
			reportWarning("Stopping the plugin framework timed out");
			return false;
		}
	}
	catch (const ctkRuntimeException& e)
	{
		reportWarning(QString("Stopping the plugin framework failed: %1").arg(e.what()));
		return false;
	}

	return !this->frameworkStarted();
}

void PluginFrameworkManager::uninstall(const QString& symbolicName)
{
	QString pluginPath = getPluginPath(symbolicName);
	if (pluginPath.isEmpty())
		return;

	try
	{
		ctkPluginContext* pc = this->getPluginContext();
		pc->installPlugin(QUrl::fromLocalFile(pluginPath))->uninstall();
	}
	catch (const ctkPluginException& exc)
	{
		reportWarning(QString("Failed to uninstall plugin: %1, %2").arg(symbolicName).arg(exc.what()));
		return;
	}

	return;
}

bool PluginFrameworkManager::start(const QString& symbolicName, ctkPlugin::StartOptions options)
{
	this->startFramework();

	QString pluginPath = getPluginPath(symbolicName);
	if (pluginPath.isEmpty())
		return false;

	try
	{
		ctkPluginContext* pc = this->getPluginContext();
		pc->installPlugin(QUrl::fromLocalFile(pluginPath))->start(options);
	}
	catch (const ctkPluginException& exc)
	{
		reportWarning(QString("Failed to start plugin: %1, %2").arg(symbolicName).arg(exc.what()));
		return false;
	}

	return true;
}

bool PluginFrameworkManager::stop(const QString& symbolicName, ctkPlugin::StopOptions options)
{
	if (!this->frameworkStarted())
		return false;
	QString pluginPath = this->getPluginPath(symbolicName);
	if (pluginPath.isEmpty())
		return false;

	QSharedPointer<ctkPlugin> plugin = this->getInstalledPluginFromSymbolicName(symbolicName);

	if (!plugin)
	{
		reportWarning(QString("Plugin: %1 not found").arg(symbolicName));
		return false;
	}

	try
	{
		plugin->stop(options);
	}
	catch (const ctkPluginException& exc)
	{
		reportWarning(QString("Failed to stop plugin %1: ").arg(symbolicName).arg(exc.what()));
		return false;
	}

	return true;
}

QSharedPointer<ctkPlugin> PluginFrameworkManager::getInstalledPluginFromSymbolicName(QString symbolicName)
{
	QSharedPointer<ctkPlugin> empty;

	if (!this->frameworkInitialized())
		return empty;

	QString pluginPath = this->getPluginPath(symbolicName);
	if (pluginPath.isEmpty())
		return empty;

	ctkPluginContext* pc = this->getPluginContext();
	QList < QSharedPointer<ctkPlugin> > plugins = pc->getPlugins();
	foreach(QSharedPointer<ctkPlugin> plugin, plugins)
	{
		if (plugin->getSymbolicName() == symbolicName)
		{
			return plugin;
		}
	}

	return empty;
}


QString PluginFrameworkManager::getPluginPath(const QString& symbolicName)
{
	QString pluginFileName(symbolicName);
	pluginFileName.replace(".", "_");
	foreach(QString searchPath, mPluginSearchPaths)
	{
		QDirIterator dirIter(searchPath, mPluginLibFilter, QDir::Files, QDirIterator::Subdirectories);
		while(dirIter.hasNext())
		{
			dirIter.next();
			QFileInfo fileInfo = dirIter.fileInfo();
			QString fileBaseName = fileInfo.baseName();
			if (fileBaseName.startsWith("lib")) fileBaseName = fileBaseName.mid(3);

			if (fileBaseName == pluginFileName)
			{
				return fileInfo.canonicalFilePath();
			}
		}
	}

	return QString();
}

QStringList PluginFrameworkManager::getPluginSymbolicNames()
{
	QStringList result;
	foreach(QString searchPath, mPluginSearchPaths)
	{
		result.append(this->getPluginSymbolicNames(searchPath));
	}
	return result;
}

QStringList PluginFrameworkManager::getPluginSymbolicNames(const QString& searchPath)
{
	QStringList result;
	QDirIterator dirIter(searchPath, mPluginLibFilter, QDir::Files, QDirIterator::Subdirectories);
	while (dirIter.hasNext())
	{
		dirIter.next();
		QFileInfo fileInfo = dirIter.fileInfo();
		QString fileBaseName = fileInfo.baseName();
		if (fileBaseName.startsWith("lib"))
			fileBaseName = fileBaseName.mid(3);
		QString name = fileBaseName.replace("_", ".");
		if (this->nameIsProbablyPlugin(name))
			result << name;
	}

	return result;
}

bool PluginFrameworkManager::nameIsProbablyPlugin(QString name) const
{
	// heuristic check for plugin-ish name
	if (name.count(".")<2) // some libs contain a _, they generate too much spam in installed version
		return false;
	if (name.contains("cxtest"))
		return false;
	return true;

}

} /* namespace cx */