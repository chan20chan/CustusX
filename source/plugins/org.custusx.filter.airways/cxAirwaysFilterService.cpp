/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.
                 
Copyright (c) SINTEF Department of Medical Technology.
All rights reserved.
                 
CustusX is released under a BSD 3-Clause license.
                 
See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
=========================================================================*/

#include "cxAirwaysFilterService.h"

#include <QTimer>

#include <vtkImageImport.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <ctkPluginContext.h>
#include <vtkImplicitModeller.h>
#include <vtkContourFilter.h>
#include "cxBranchList.h"
#include "cxBronchoscopyRegistration.h"
#include "cxAirwaysFromCenterline.h"
#include "cxVolumeHelpers.h"

#include "cxTime.h"
#include "cxTypeConversions.h"
#include "cxLogger.h"
#include "cxRegistrationTransform.h"
#include "cxDoubleProperty.h"
#include "cxContourFilter.h"
#include "cxImage.h"
#include "cxDataLocations.h"
#include "cxSelectDataStringProperty.h"
#include "vtkForwardDeclarations.h"
#include "cxPatientModelServiceProxy.h"
#include "cxVisServices.h"
#include "cxUtilHelpers.h"
#include "FAST/Algorithms/LungSegmentation/LungSegmentation.hpp"
#include "FAST/Algorithms/AirwaySegmentation/AirwaySegmentation.hpp"
#include "FAST/Algorithms/CenterlineExtraction/CenterlineExtraction.hpp"
#include "FAST/Importers/ImageFileImporter.hpp"
#include "FAST/Exporters/VTKImageExporter.hpp"
#include "FAST/Exporters/VTKMeshExporter.hpp"
#include "FAST/Data/Segmentation.hpp"
#include "FAST/SceneGraph.hpp"

namespace cx {

AirwaysFilter::AirwaysFilter(VisServicesPtr services) :
	FilterImpl(services)
{
	fast::Reporter::setGlobalReportMethod(fast::Reporter::COUT);
    //Need to create OpenGL context of fast in main thread, this is done in the constructor of DeviceManger
    fast::ImageFileImporter::pointer importer = fast::ImageFileImporter::New();
    Q_UNUSED(importer)
}


AirwaysFilter::~AirwaysFilter() {
}

QString AirwaysFilter::getName() const
{
	return "Airway Segmentation Filter";
}

QString AirwaysFilter::getType() const
{
	return "airways_filter";
}

QString AirwaysFilter::getHelp() const
{
	return "<html>"
	        "<h3>Airway Segmentation.</h3>"
					"<p><i>Extracts airways and blood vessels, including centerlines, and lungs from a CT volume. If method fails, try to crop volume. </br>Algorithm written by Erik Smistad.</i></p>"
           "</html>";
}

QString AirwaysFilter::getNameSuffixCenterline()
{
    return "_centerline";
}

QString AirwaysFilter::getNameSuffixAirways()
{
	return "_airways";
}

QString AirwaysFilter::getNameSuffixTubes()
{
	return "_tubes";
}

QString AirwaysFilter::getNameSuffixLungs()
{
	return "_lungs";
}

QString AirwaysFilter::getNameSuffixVessels()
{
	return "_vessels";
}

QString AirwaysFilter::getNameSuffixVolume()
{
	return "_volume";
}

Vector3D AirwaysFilter::getSeedPointFromTool(SpaceProviderPtr spaceProvider, DataPtr data)
{
	// Retrieve position of tooltip and use it as seed point
	Vector3D point = spaceProvider->getActiveToolTipPoint(
			spaceProvider->getD(data));

	// Have to multiply by the inverse of the spacing to get the voxel position
	ImagePtr image = boost::dynamic_pointer_cast<Image>(data);
	double spacingX, spacingY, spacingZ;
	image->getBaseVtkImageData()->GetSpacing(spacingX, spacingY, spacingZ);
	point(0) = point(0) * (1.0 / spacingX);
	point(1) = point(1) * (1.0 / spacingY);
	point(2) = point(2) * (1.0 / spacingZ);

	std::cout << "the selected seed point is: " << point(0) << " " << point(1)
			<< " " << point(2) << "\n";

	return point;
}

int * getImageSize(DataPtr inputImage)
{
	ImagePtr image = boost::dynamic_pointer_cast<Image>(inputImage);
	return image->getBaseVtkImageData()->GetDimensions();
}

bool AirwaysFilter::isSeedPointInsideImage(Vector3D seedPoint, DataPtr image)
{
	int * size = getImageSize(image);
	std::cout << "size of image is: " << size[0] << " " << size[1] << " "
			<< size[2] << "\n";
	int x = (int) seedPoint(0);
	int y = (int) seedPoint(1);
	int z = (int) seedPoint(2);
	bool result = x >= 0 && y >= 0 && z >= 0 && x < size[0] && y < size[1]
			&& z < size[2];
	return result;
}

bool AirwaysFilter::preProcess()
{
    DataPtr inputImage = mInputTypes[0].get()->getData();
	if (!inputImage)
	{
		CX_LOG_ERROR() << "No input data selected";
		return false;
	}

	if (inputImage->getType() != Image::getTypeName())
	{
		CX_LOG_ERROR() << "Input data has to be an image";
		return false;
	}

	std::string filename = (patientService()->getActivePatientFolder()
			+ "/" + inputImage->getFilename()).toStdString();

    // only check seed point inside image if use seed point is checked
	bool useManualSeedPoint = getManualSeedPointOption(mOptions)->getValue();
	if(useManualSeedPoint)
	{
		seedPoint = getSeedPointFromTool(mServices->spaceProvider(), inputImage);
		if(!isSeedPointInsideImage(seedPoint, inputImage)) {
			CX_LOG_ERROR() << "Seed point is not inside image. Use cursor to set seed point inside trachea in the CT image.";
			return false;
		}
	}
	mInputImage = patientService()->getData<Image>(inputImage->getUid());

	return true;
}

bool AirwaysFilter::execute()
{

	CX_LOG_INFO() << "EXECUTING AIRWAYS FILTER";
	// Check if pre process went ok:
	if(!mInputImage)
		return false;

	QString q_filename = "";
	QString activePatienFolder = patientService()->getActivePatientFolder();
	QString inputImageFileName = mInputImage->getFilename();
	if(!activePatienFolder.isEmpty())
		q_filename = activePatienFolder+"/"+inputImageFileName;
	else
		q_filename = inputImageFileName;

	try {
		fast::Config::getTestDataPath(); // needed for initialization
		QString cacheDir = cx::DataLocations::getCachePath();
		fast::Config::setKernelBinaryPath(cacheDir.toStdString());
		QString kernelDir = cx::DataLocations::findConfigFolder("/FAST", FAST_SOURCE_DIR);
		fast::Config::setKernelSourcePath(kernelDir.toStdString());


	} catch(fast::Exception& e) {
		std::string error = e.what();
		reportError("fast::Exception: "+qstring_cast(error));

		return false;

	} catch (...){
		reportError("Airway segmentation algorithm threw a unknown exception.");

		return false;
	}

	bool doAirwaySegmentation = getAirwaySegmentationOption(mOptions)->getValue();
	bool doLungSegmentation = getLungSegmentationOption(mOptions)->getValue();
	bool doVesselSegmentation = getVesselSegmentationOption(mOptions)->getValue();

	if (doAirwaySegmentation)
	{
		std::string volumeFilname = q_filename.toStdString();
		// Import image data from disk
		fast::ImageFileImporter::pointer importerPtr = fast::ImageFileImporter::New();
		importerPtr->setFilename(volumeFilname);

		segmentAirways(importerPtr);
	}

	if (doLungSegmentation)
	{
		std::string volumeFilname = q_filename.toStdString();
		// Import image data from disk
		fast::ImageFileImporter::pointer importerPtr = fast::ImageFileImporter::New();
		importerPtr->setFilename(volumeFilname);

		segmentLungs(importerPtr);
	}

	if (doVesselSegmentation)
	{
		std::string volumeFilname = q_filename.toStdString();
		// Import image data from disk
		fast::ImageFileImporter::pointer importerPtr = fast::ImageFileImporter::New();
		importerPtr->setFilename(volumeFilname);

		segmentVessels(importerPtr);
	}

	return true;
}

void AirwaysFilter::segmentAirways(fast::ImageFileImporter::pointer importerPtr)
{

	bool useManualSeedPoint = getManualSeedPointOption(mOptions)->getValue();


	// Do segmentation
	fast::AirwaySegmentation::pointer airwaySegmentationPtr = fast::AirwaySegmentation::New();

	airwaySegmentationPtr->setInputConnection(importerPtr->getOutputPort());

	if(useManualSeedPoint) {
		CX_LOG_INFO() << "Using seed point: " << seedPoint.transpose();
		airwaySegmentationPtr->setSeedPoint(seedPoint(0), seedPoint(1), seedPoint(2));
	}

	extractAirways(airwaySegmentationPtr);
}

bool AirwaysFilter::extractAirways(fast::AirwaySegmentation::pointer airwaySegmentationPtr)
{

	try{
		auto segPort = airwaySegmentationPtr->getOutputPort();

		// Convert fast segmentation data to VTK data which CX can use
		vtkSmartPointer<fast::VTKImageExporter> vtkExporter = fast::VTKImageExporter::New();
		vtkExporter->setInputConnection(airwaySegmentationPtr->getOutputPort());
		vtkExporter->Update();
		mAirwaySegmentationOutput = vtkExporter->GetOutput();
		vtkExporter->Delete();

		auto airwaySegmentationData = segPort->getNextFrame<fast::SpatialDataObject>();

		// Extract centerline
		fast::CenterlineExtraction::pointer centerline = fast::CenterlineExtraction::New();
		centerline->setInputData(airwaySegmentationData);
		// Get centerline
		vtkSmartPointer<fast::VTKMeshExporter> vtkCenterlineExporter = fast::VTKMeshExporter::New();
		vtkCenterlineExporter->setInputConnection(centerline->getOutputPort());
		vtkCenterlineExporter->Update();
		mAirwayCenterlineOutput = vtkCenterlineExporter->GetOutput();
		vtkCenterlineExporter->Delete();

} catch(fast::Exception& e) {
	std::string error = e.what();
	reportError("fast::Exception: "+qstring_cast(error));
	if(!getManualSeedPointOption(mOptions)->getValue())
		CX_LOG_ERROR() << "Try to set the seed point manually.";

	return false;

} catch(cl::Error& e) {
	reportError("cl::Error:"+qstring_cast(e.what()));

	return false;

} catch (std::exception& e){
	reportError("std::exception:"+qstring_cast(e.what()));
	std::cout << "DEBUG std::exception" << std::endl;  //debug
	return false;

} catch (...){
	reportError("Airway segmentation algorithm threw a unknown exception.");

	return false;
}

	return true;

}

void AirwaysFilter::segmentLungs(fast::ImageFileImporter::pointer importerPtr)
{

	bool useManualSeedPoint = getManualSeedPointOption(mOptions)->getValue();

	// Do segmentation
	fast::LungSegmentation::pointer lungSegmentationPtr = fast::LungSegmentation::New();
	lungSegmentationPtr->setInputConnection(importerPtr->getOutputPort());

	if(useManualSeedPoint) {
		CX_LOG_INFO() << "Using seed point: " << seedPoint.transpose();
		lungSegmentationPtr->setAirwaySeedPoint(seedPoint(0), seedPoint(1), seedPoint(2));
	}

	extractLungs(lungSegmentationPtr);
}

void AirwaysFilter::segmentVessels(fast::ImageFileImporter::pointer importerPtr)
{

	bool useManualSeedPoint = getManualSeedPointOption(mOptions)->getValue();

	// Do segmentation
	fast::LungSegmentation::pointer lungSegmentationPtr = fast::LungSegmentation::New();
	lungSegmentationPtr->setInputConnection(importerPtr->getOutputPort());

	if(useManualSeedPoint) {
		CX_LOG_INFO() << "Using seed point: " << seedPoint.transpose();
		lungSegmentationPtr->setAirwaySeedPoint(seedPoint(0), seedPoint(1), seedPoint(2));
	}

	extractBloodVessels(lungSegmentationPtr);
}

bool AirwaysFilter::extractBloodVessels(fast::LungSegmentation::pointer lungSegmentationPtr)
{
	try{
		auto segPortBloodVessels = lungSegmentationPtr->getBloodVesselOutputPort();

		// Convert fast segmentation data to VTK data which CX can use
		vtkSmartPointer<fast::VTKImageExporter> vtkBloodVesselExporter = fast::VTKImageExporter::New();
		vtkBloodVesselExporter->setInputConnection(lungSegmentationPtr->getBloodVesselOutputPort());
		vtkBloodVesselExporter->Update();
		mBloodVesselSegmentationOutput = vtkBloodVesselExporter->GetOutput();
		vtkBloodVesselExporter->Delete();

		auto bloodVesselSegmentationData = segPortBloodVessels->getNextFrame<fast::SpatialDataObject>();

		// Extract centerline
		fast::CenterlineExtraction::pointer bloodVesselCenterline = fast::CenterlineExtraction::New();
		bloodVesselCenterline->setInputData(bloodVesselSegmentationData);

		// Get centerline
		vtkSmartPointer<fast::VTKMeshExporter> vtkBloodVesselCenterlineExporter = fast::VTKMeshExporter::New();
		vtkBloodVesselCenterlineExporter->setInputConnection(bloodVesselCenterline->getOutputPort());
		vtkBloodVesselCenterlineExporter->Update();
		mBloodVesselCenterlineOutput = vtkBloodVesselCenterlineExporter->GetOutput();
		vtkBloodVesselCenterlineExporter->Delete();

} catch(fast::Exception& e) {
	std::string error = e.what();
	reportError("In vessel segmentation fast::Exception: "+qstring_cast(error));
	if(!getManualSeedPointOption(mOptions)->getValue())
		CX_LOG_ERROR() << "Try to set the seed point manually.";

	return false;

} catch(cl::Error& e) {
	reportError("In vessel segmentation cl::Error:"+qstring_cast(e.what()));

	return false;

} catch (std::exception& e){
	reportError("In vessel segmentation std::exception:"+qstring_cast(e.what()));

	return false;

} catch (...){
	reportError("Vessel segmentation algorithm threw a unknown exception.");

	return false;
}

	return true;
}

bool AirwaysFilter::extractLungs(fast::LungSegmentation::pointer lungSegmentationPtr)
{
	try{
		// Convert fast segmentation data to VTK data which CX can use
		vtkSmartPointer<fast::VTKImageExporter> vtkLungExporter = fast::VTKImageExporter::New();
		vtkLungExporter->setInputConnection(lungSegmentationPtr->getOutputPort(0));
		vtkLungExporter->Update();
		mLungSegmentationOutput = vtkLungExporter->GetOutput();
		vtkLungExporter->Delete();

} catch(fast::Exception& e) {
	std::string error = e.what();
	reportError("In lung segmentation fast::Exception: "+qstring_cast(error));
	if(!getManualSeedPointOption(mOptions)->getValue())
		CX_LOG_ERROR() << "Try to set the seed point manually.";

	return false;

} catch(cl::Error& e) {
	reportError("In lung segmentation cl::Error:"+qstring_cast(e.what()));

	return false;

} catch (std::exception& e){
	reportError("In lung segmentation std::exception:"+qstring_cast(e.what()));

	return false;

} catch (...){
	reportError("Lung segmentation algorithm threw a unknown exception.");

	return false;
}

	return true;
}

bool AirwaysFilter::postProcess()
{
	std::cout << "POST PROCESS" << std::endl;

	if(getAirwaySegmentationOption(mOptions)->getValue())
	{
		postProcessAirways();
		mAirwaySegmentationOutput = NULL; //To avoid publishing old results if next segmentation fails
		mAirwayCenterlineOutput = NULL;
	}

	if(getLungSegmentationOption(mOptions)->getValue()) {
		postProcessLungs();
		mLungSegmentationOutput = NULL; //To avoid publishing old results if next segmentation fails
	}

	if(getVesselSegmentationOption(mOptions)->getValue())
	{
		postProcessVessels();
		mBloodVesselSegmentationOutput = NULL; //To avoid publishing old results if next segmentation fails
		mBloodVesselCenterlineOutput = NULL;
	}

	return true;
}

bool AirwaysFilter::postProcessAirways()
{
	if(!mAirwaySegmentationOutput)
		return false;

	// Make contour of segmented volume
	double threshold = 1; /// because the segmented image is 0..1
	vtkPolyDataPtr rawContour = ContourFilter::execute(
			mAirwaySegmentationOutput,
			threshold,
			false, // reduce resolution
			true, // smoothing
			true, // keep topology
			0 // target decimation
	);

	//Create temporary ImagePtr for correct output name from contour filter
	QString uidOutput = mInputImage->getUid() + AirwaysFilter::getNameSuffixAirways() + "%1";
	QString nameOutput = mInputImage->getName() + AirwaysFilter::getNameSuffixAirways() + "%1";
	ImagePtr outputImage = patientService()->createSpecificData<Image>(uidOutput, nameOutput);
	// Add contour internally to cx
	MeshPtr contour = ContourFilter::postProcess(
			patientService(),
			rawContour,
			outputImage,
			QColor("green")
	);
	contour->get_rMd_History()->setRegistration(mInputImage->get_rMd());

	// Set output
	mOutputTypes[1]->setValue(contour->getUid());

	// Centerline
	QString uid = mInputImage->getUid() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixCenterline() + "%1";
	QString name = mInputImage->getName() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixCenterline() + "%1";
	MeshPtr airwaysCenterline = patientService()->createSpecificData<Mesh>(uid, name);
	airwaysCenterline->setVtkPolyData(mAirwayCenterlineOutput);
	airwaysCenterline->get_rMd_History()->setParentSpace(mInputImage->getUid());
	airwaysCenterline->get_rMd_History()->setRegistration(mInputImage->get_rMd());
	patientService()->insertData(airwaysCenterline);
	mOutputTypes[0]->setValue(airwaysCenterline->getUid());

	if(getAirwayTubesGenerationOption(mOptions)->getValue())
		this->createAirwaysFromCenterline();

	return true;
}

bool AirwaysFilter::postProcessLungs()
{
	if(!mLungSegmentationOutput)
		return false;

	double threshold = 1; /// because the segmented image is 0..1
	vtkPolyDataPtr rawContour = ContourFilter::execute(
			mLungSegmentationOutput,
			threshold,
			false, // reduce resolution
			true, // smoothing
			true, // keep topology
			0 // target decimation
	);

	//Create temporary ImagePtr for correct output name from contour filter
	QString uidOutput = mInputImage->getUid() + AirwaysFilter::getNameSuffixLungs() + "%1";
	QString nameOutput = mInputImage->getName() + AirwaysFilter::getNameSuffixLungs() + "%1";
	ImagePtr outputImage = patientService()->createSpecificData<Image>(uidOutput, nameOutput);

	 //Add contour internally to cx
	QColor color("red");
	color.setAlpha(100);
	MeshPtr contour = ContourFilter::postProcess(
			patientService(),
			rawContour,
			outputImage,
			color
	);

	contour->get_rMd_History()->setRegistration(mInputImage->get_rMd());

	// Set output
	mOutputTypes[4]->setValue(contour->getUid());

	return true;
}

bool AirwaysFilter::postProcessVessels()
{
	if(!mBloodVesselSegmentationOutput)
		return false;

	// Make contour of segmented volume
	double threshold = 1; /// because the segmented image is 0..1
	vtkPolyDataPtr rawContour = ContourFilter::execute(
			mBloodVesselSegmentationOutput,
			threshold,
			false, // reduce resolution
			true, // smoothing
			true, // keep topology
			0 // target decimation
	);

	//Create temporary ImagePtr for correct output name from contour filter
	QString uidOutput = mInputImage->getUid() + AirwaysFilter::getNameSuffixVessels() + "%1";
	QString nameOutput = mInputImage->getName() + AirwaysFilter::getNameSuffixVessels() + "%1";
	ImagePtr outputImage = patientService()->createSpecificData<Image>(uidOutput, nameOutput);

	// Add contour internally to cx
	MeshPtr contour = ContourFilter::postProcess(
			patientService(),
			rawContour,
			outputImage,
			QColor("blue")
	);
	contour->get_rMd_History()->setRegistration(mInputImage->get_rMd());

	// Set output
	mOutputTypes[6]->setValue(contour->getUid());

	// Centerline
	QString uid = mInputImage->getUid() + AirwaysFilter::getNameSuffixVessels() + AirwaysFilter::getNameSuffixCenterline() + "%1";
	QString name = mInputImage->getName() + AirwaysFilter::getNameSuffixVessels() + AirwaysFilter::getNameSuffixCenterline() + "%1";
	MeshPtr bloodVesselsCenterline = patientService()->createSpecificData<Mesh>(uid, name);
	bloodVesselsCenterline->setVtkPolyData(mBloodVesselCenterlineOutput);
	bloodVesselsCenterline->get_rMd_History()->setParentSpace(mInputImage->getUid());
	bloodVesselsCenterline->get_rMd_History()->setRegistration(mInputImage->get_rMd());
	bloodVesselsCenterline->setColor("blue");
	patientService()->insertData(bloodVesselsCenterline);
	mOutputTypes[5]->setValue(bloodVesselsCenterline->getUid());


	//Create segmented volume output
	QString uidVolume = mInputImage->getUid() + AirwaysFilter::getNameSuffixVessels() + AirwaysFilter::getNameSuffixVolume() + "%1";
	QString nameVolume =  mInputImage->getName() + AirwaysFilter::getNameSuffixVessels() + AirwaysFilter::getNameSuffixVolume() + "%1";
	ImagePtr outputVolume = createDerivedImage(mServices->patient(),
	                                         uidVolume, nameVolume,
											 mBloodVesselSegmentationOutput, mInputImage);
	outputVolume->mergevtkSettingsIntosscTransform();
	patientService()->insertData(outputVolume);
	mOutputTypes[7]->setValue(outputVolume->getUid());

	return true;
}

void AirwaysFilter::createAirwaysFromCenterline()
{
    AirwaysFromCenterlinePtr airwaysFromCLPtr = AirwaysFromCenterlinePtr(new AirwaysFromCenterline());

    airwaysFromCLPtr->processCenterline(mAirwayCenterlineOutput);

    // Create the mesh object from the airway walls
    QString uidMesh = mInputImage->getUid() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixTubes() + "%1";
    QString nameMesh = mInputImage->getName() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixTubes() + "%1";
    MeshPtr airwayWalls = patientService()->createSpecificData<Mesh>(uidMesh, nameMesh);
    airwayWalls->setVtkPolyData(airwaysFromCLPtr->generateTubes());
    airwayWalls->get_rMd_History()->setParentSpace(mInputImage->getUid());
    airwayWalls->get_rMd_History()->setRegistration(mInputImage->get_rMd());
    airwayWalls->setColor(QColor(253, 173, 136, 255));
    patientService()->insertData(airwayWalls);
    mOutputTypes[3]->setValue(airwayWalls->getUid());


    //insert filtered centerline from airwaysFromCenterline
    QString uidCenterline = mInputImage->getUid() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixTubes() + AirwaysFilter::getNameSuffixCenterline() + "%1";
    QString nameCenterline = mInputImage->getName() + AirwaysFilter::getNameSuffixAirways() + AirwaysFilter::getNameSuffixTubes() + AirwaysFilter::getNameSuffixCenterline() + "%1";
    MeshPtr centerline = patientService()->createSpecificData<Mesh>(uidCenterline, nameCenterline);
    centerline->setVtkPolyData(airwaysFromCLPtr->getVTKPoints());
    centerline->get_rMd_History()->setParentSpace(mInputImage->getUid());
    centerline->get_rMd_History()->setRegistration(mInputImage->get_rMd());
    patientService()->insertData(centerline);
    mOutputTypes[2]->setValue(centerline->getUid());
}


void AirwaysFilter::createOptions()
{
	mOptionsAdapters.push_back(this->getManualSeedPointOption(mOptions));
	mOptionsAdapters.push_back(this->getAirwaySegmentationOption(mOptions));
	mOptionsAdapters.push_back(this->getAirwayTubesGenerationOption(mOptions));
	mOptionsAdapters.push_back(this->getLungSegmentationOption(mOptions));
	mOptionsAdapters.push_back(this->getVesselSegmentationOption(mOptions));
}

void AirwaysFilter::createInputTypes()
{
	SelectDataStringPropertyBasePtr temp;

	temp = StringPropertySelectImage::New(patientService());
	temp->setValueName("Input");
	temp->setHelp("Select input to run airway segmentation on.");
	mInputTypes.push_back(temp);
}

void AirwaysFilter::createOutputTypes()
{
	StringPropertySelectMeshPtr tempMeshStringAdapter;
	std::vector<std::pair<QString, QString>> valueHelpPairs;
	valueHelpPairs.push_back(std::make_pair(tr("Airway Centerline"), tr("Generated centerline mesh (vtk-format).")));
	valueHelpPairs.push_back(std::make_pair(tr("Airway Segmentation Mesh"), tr("Generated surface of the airway segmentation volume.")));
	valueHelpPairs.push_back(std::make_pair(tr("Straight Airway Centerline"), tr("Centerlines with straight lines between the branch points.")));
	valueHelpPairs.push_back(std::make_pair(tr("Straight Airway Tubes Mesh"), tr("Tubes based on the straight centerline")));
	valueHelpPairs.push_back(std::make_pair(tr("Lung Segmentation Mesh"), tr("Generated surface of the lung segmentation volume.")));
	valueHelpPairs.push_back(std::make_pair(tr("Blood Vessel Centerlines"), tr("Segmented blood vessel centerlines.")));
	valueHelpPairs.push_back(std::make_pair(tr("Blood Vessels Mesh"), tr("Segmented blood vessels in the lungs.")));

	foreach(auto pair, valueHelpPairs)
	{
		tempMeshStringAdapter = StringPropertySelectMesh::New(patientService());
		tempMeshStringAdapter->setValueName(pair.first);
		tempMeshStringAdapter->setHelp(pair.second);
		mOutputTypes.push_back(tempMeshStringAdapter);
	}

	StringPropertySelectImagePtr tempVolumeStringAdapter = StringPropertySelectImage::New(patientService());
	tempVolumeStringAdapter->setValueName("Blood Vessels Volume");
	tempVolumeStringAdapter->setHelp("Volume of segmented blood vessels in the lungs.");
	mOutputTypes.push_back(tempVolumeStringAdapter);
}


BoolPropertyPtr AirwaysFilter::getManualSeedPointOption(QDomElement root)
{
	BoolPropertyPtr retval =
			BoolProperty::initialize("Use manual seed point",
					"",
					"If the automatic seed point detection algorithm fails you can use cursor to set the seed point "
					"inside trachea of the patient. "
					"Then tick this checkbox to use the manual seed point in the airways filter.",
					false, root);
	return retval;

}

BoolPropertyPtr AirwaysFilter::getAirwaySegmentationOption(QDomElement root)
{
	BoolPropertyPtr retval =
			BoolProperty::initialize("Airway segmentation",
					"",
					"Selecting this option will segment airways",
					false, root);
	return retval;

}

BoolPropertyPtr AirwaysFilter::getAirwayTubesGenerationOption(QDomElement root)
{
	BoolPropertyPtr retval =
			BoolProperty::initialize("Airway tubes generation",
					"",
					"Selecting this option will generate artificial airway tubes for virtual bronchoscopy",
					false, root);
	return retval;

}

BoolPropertyPtr AirwaysFilter::getLungSegmentationOption(QDomElement root)
{
	BoolPropertyPtr retval =
			BoolProperty::initialize("Lung segmentation",
					"",
					"Selecting this option will segment the two lung sacs",
					false, root);
	return retval;

}

BoolPropertyPtr AirwaysFilter::getVesselSegmentationOption(QDomElement root)
{
	BoolPropertyPtr retval =
			BoolProperty::initialize("Vessel segmentation",
					"",
					"Selecting this option will segment the blood vessels in the lungs",
					false, root);
	return retval;

}

} /* namespace cx */

