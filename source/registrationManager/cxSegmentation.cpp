/*
 * cxSegmentation.cpp
 *
 *  Created on: Oct 12, 2010
 *      Author: christiana
 */

#include "cxSegmentation.h"

//ITK
#include <itkImage.h>
//#include <itkVTKImageImport.h>
//#include <itkImageToVTKImageFilter.h>
//#include <itkVTKImageToImageFilter.h>
#include <itkSmoothingRecursiveGaussianImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
//#include <itkBinaryThinningImageFilter.h>

#include "ItkVtkGlue/itkImageToVTKImageFilter.h"
#include "ItkVtkGlue/itkVTKImageToImageFilter.h"

//VTK
#include <vtkSmartPointer.h>
//#include <vtkImageExport.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyData.h>

typedef vtkSmartPointer<vtkImageExport> vtkImageExportPtr;

#include "sscTypeConversions.h"
#include "sscRegistrationTransform.h"
#include "sscImage.h"
#include "sscDataManager.h"
#include "sscUtilHelpers.h"
#include "sscMesh.h"
#include "sscMessageManager.h"

const unsigned int Dimension = 3;
typedef unsigned short PixelType;
typedef itk::Image< PixelType, Dimension >  itkImageType;
typedef itk::ImageToVTKImageFilter<itkImageType> itkToVtkFilterType;
typedef itk::VTKImageToImageFilter<itkImageType> itkVTKImageToImageFilterType;

typedef vtkSmartPointer<class vtkMarchingCubes> vtkMarchingCubesPtr;
typedef vtkSmartPointer<class vtkPolyData> vtkPolyDataPtr;

namespace cx
{

itkImageType::ConstPointer getITKfromSSCImage(ssc::ImagePtr image)
{
  itkVTKImageToImageFilterType::Pointer vtk2itkFilter = itkVTKImageToImageFilterType::New();
  //itkToVtkFilter->SetInput(data);
  vtk2itkFilter->SetInput(image->getBaseVtkImageData());
  vtk2itkFilter->Update();
  return vtk2itkFilter->GetOutput();
}

void Segmentation::contour(ssc::ImagePtr image, QString outputBasePath, int threshold)
{
  //itkImageType::ConstPointer itkImage = getITKfromSSCImage(image);

    //Create vtkPolyData
    /*vtkImageToPolyDataFilter* convert = vtkImageToPolyDataFilter::New();
    convert->SetInput(itkToVtkFilter->GetOutput());
    convert->SetColorModeToLinear256();
    convert->Update();*/

    vtkMarchingCubesPtr convert = vtkMarchingCubesPtr::New();
    convert->SetInput(image->getBaseVtkImageData());
    //convert->SetValue(0, threshold);
    convert->Update();
    //messageManager()->sendInfo("Number of contours: "+QString::number(convert->GetNumberOfContours()).toStdString());

    vtkPolyDataPtr cubesPolyData = vtkPolyDataPtr::New();
    cubesPolyData = convert->GetOutput();


//    vtkImageDataPtr rawResult = vtkImageDataPtr::New();
//    rawResult->DeepCopy(itkToVtkFilter->GetOutput());
//    // TODO: possible memory problem here - check debug mem system of itk/vtk


    std::string uid = string_cast(ssc::changeExtension(qstring_cast(image->getUid()), "") + "_cont%1");
    std::string name = image->getName()+" contour %1";
    //std::cout << "contoured volume: " << uid << ", " << name << std::endl;
    ssc::MeshPtr result = ssc::dataManager()->createMesh(cubesPolyData,uid, name, "Images");
    ssc::messageManager()->sendInfo("created contour " + result->getName());

    result->get_rMd_History()->setRegistration(image->get_rMd());
    result->setParentFrame(image->getUid());

    ssc::dataManager()->loadData(result);
    //ssc::dataManager()->saveImage(result, string_cast(outputBasePath));

//
//    //print
//    //itkToVtkFilter->GetOutput()->Print(std::cout);
//    //cubesPolyData->Print(std::cout);
//    //vtkPolyData* cubesPolyData = convert->GetOutput();
//
//    ssc::MeshPtr surface = ssc::MeshPtr(new ssc::Mesh(outName.toStdString()+"_segm"));
//    surface->setVtkPolyData(cubesPolyData);
//    ssc::GeometricRepPtr surfaceRep(ssc::GeometricRep::New(outName.toStdString()+"_segm"));
//    surfaceRep->setMesh(surface);

}

void Segmentation::segment(ssc::ImagePtr image, QString outputBasePath, int threshold, bool useSmothing, double smoothSigma)
{
  itkImageType::ConstPointer itkImage = getITKfromSSCImage(image);

  //Smoothing
  if(useSmothing)
  {
    typedef itk::SmoothingRecursiveGaussianImageFilter<itkImageType, itkImageType> smoothingFilterType;
    smoothingFilterType::Pointer smoohingFilter = smoothingFilterType::New();
    smoohingFilter->SetSigma(smoothSigma);
    smoohingFilter->SetInput(itkImage);
    smoohingFilter->Update();
    itkImage = smoohingFilter->GetOutput();
  }

  //Thresholding
  typedef itk::BinaryThresholdImageFilter<itkImageType, itkImageType> thresholdFilterType;
  thresholdFilterType::Pointer thresholdFilter = thresholdFilterType::New();
  thresholdFilter->SetInput(itkImage);
  //TODO:  support non-binary images
  thresholdFilter->SetOutsideValue(0);
  thresholdFilter->SetInsideValue(1);
  thresholdFilter->SetLowerThreshold(threshold);
  thresholdFilter->Update();
  itkImage = thresholdFilter->GetOutput();

  //Convert ITK to VTK
  itkToVtkFilterType::Pointer itkToVtkFilter = itkToVtkFilterType::New();
  itkToVtkFilter->SetInput(itkImage);
  itkToVtkFilter->Update();

  vtkImageDataPtr rawResult = vtkImageDataPtr::New();
  rawResult->DeepCopy(itkToVtkFilter->GetOutput());
  // TODO: possible memory problem here - check debug mem system of itk/vtk


  std::string uid = string_cast(ssc::changeExtension(qstring_cast(image->getUid()), "") + "_seg%1");
  std::string name = image->getName()+" segmented %1";
  //std::cout << "segmented volume: " << uid << ", " << name << std::endl;
  ssc::ImagePtr result = ssc::dataManager()->createImage(rawResult,uid, name);
  ssc::messageManager()->sendInfo("created segment " + result->getName());

  result->get_rMd_History()->setRegistration(image->get_rMd());

  result->setParentFrame(image->getUid());
  ssc::dataManager()->loadData(result);
  ssc::dataManager()->saveImage(result, string_cast(outputBasePath));

}

} // namespace cx


//void ShiftCorrectionWidget::segmentImage(QString imageName,
//                                         int thresholdValue,
//                                         bool smoothing)
//{
//  QString filePath = mWorkingFolder+"/"+imageName;
//
//  //Read image file
//  //ITK
//  ImageReaderType::Pointer  reader  = ImageReaderType::New();
//  reader->SetFileName(filePath.toLatin1());
//  reader->Update();
//
//  ImageDirectionType direction = reader->GetOutput()->GetDirection();
//  //std::cout << "Matrix = " << std::endl << direction << std::endl;
//  int i,j;
//  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
//  for (i=0;i<3;i++)
//  {
//    for(j=0;j<3;j++)
//    {
//      matrix->SetElement(i, j, direction.GetVnlMatrix()[i][j]);
//      //std::cout << "Matrix element = " << matrix->GetElement(i,j) << std::endl;
//    }
//  }
//
//  ImageType::Pointer data = reader->GetOutput();
//
//  //Smooting
//  if(smoothing)
//  {
//    typedef itk::SmoothingRecursiveGaussianImageFilter<ImageType, ImageType> smoothingFilterType;
//    smoothingFilterType::Pointer smoohingFilter = smoothingFilterType::New();
//    smoohingFilter->SetSigma(0.5);
//    smoohingFilter->SetInput(data);
//    smoohingFilter->Update();
//    data = smoohingFilter->GetOutput();
//  }
//
//  //Thresholding
//  typedef itk::BinaryThresholdImageFilter<ImageType, ImageType> thresholdFilterType;
//  thresholdFilterType::Pointer thresholdFilter = thresholdFilterType::New();
//  thresholdFilter->SetInput(data);
//  thresholdFilter->SetOutsideValue(0);
//  thresholdFilter->SetInsideValue(1);
//  thresholdFilter->SetLowerThreshold(thresholdValue);
//  thresholdFilter->Update();
//  data = thresholdFilter->GetOutput();
//
//  //Test writer
//  QString outName = filePath;
//  if(outName.endsWith(".mhd"))
//    outName.replace(QString(".mhd"), QString(""));
//  if(outName.endsWith(".mha"))
//    outName.replace(QString(".mha"), QString(""));
//
//  QString outFileName = outName+"_segm_converted.mhd";
//  QString rawFileName = outName+"_segm_converted.raw";
//
//
//  //Convert ITK to VTK
//  itkToVtkFilterType::Pointer itkToVtkFilter = itkToVtkFilterType::New();
//  //itkToVtkFilter->SetInput(data);
//  itkToVtkFilter->SetInput(thresholdFilter->GetOutput());
//  itkToVtkFilter->Update();
//
//  //Get 3D view
//  View3D* view = ViewManager::getInstance()->get3DView();
//
//  //Show converted volume = empty?
//  /*ssc::ImagePtr image = ssc::ImagePtr(new ssc::Image(outName.toStdString()+"_segm_volume",
//                                                     itkToVtkFilter->GetOutput()));
//  ssc::VolumetricRepPtr volumetricRep(ssc::VolumetricRep::New(outName.toStdString()+"_segm_volume"));
//  volumetricRep->setImage(image);
//
//  //Crash???
//  view->setRep(volumetricRep);*/
//
//
//  //test Save vtk object
//  /*typedef vtkSmartPointer<vtkMetaImageWriter> vtkMetaImageWriterPtr;
//  vtkMetaImageWriterPtr vtkWriter = vtkMetaImageWriterPtr::New();
//  vtkWriter->SetInput(itkToVtkFilter->GetOutput());
//  vtkWriter->SetFileName( outFileName.toLatin1() );
//  vtkWriter->SetRAWFileName( rawFileName.toLatin1() );
//  vtkWriter->SetCompression(false);
//  vtkWriter->Update();
//  vtkWriter->Write();*/
//
//
//  outFileName = outName+"_segm.mhd";
//
//  //Create vtkPolyData
//  /*vtkImageToPolyDataFilter* convert = vtkImageToPolyDataFilter::New();
//  convert->SetInput(itkToVtkFilter->GetOutput());
//  convert->SetColorModeToLinear256();
//  convert->Update();*/
//
//  vtkMarchingCubesPtr convert = vtkMarchingCubesPtr::New();
//  convert->SetInput(itkToVtkFilter->GetOutput());
//  //convert->SetValue(0, 150);
//  convert->Update();
//  //messageManager()->sendInfo("Number of contours: "+QString::number(convert->GetNumberOfContours()).toStdString());
//
//  vtkPolyDataPtr cubesPolyData = vtkPolyDataPtr::New();
//  cubesPolyData = convert->GetOutput();
//
//  //print
//  //itkToVtkFilter->GetOutput()->Print(std::cout);
//  cubesPolyData->Print(std::cout);
//  //vtkPolyData* cubesPolyData = convert->GetOutput();
//
//  ssc::MeshPtr surface = ssc::MeshPtr(new ssc::Mesh(outName.toStdString()+"_segm"));
//  surface->setVtkPolyData(cubesPolyData);
//  ssc::GeometricRepPtr surfaceRep(ssc::GeometricRep::New(outName.toStdString()+"_segm"));
//  surfaceRep->setMesh(surface);
//
//  view->addRep(surfaceRep);
//
//  //Cone test
//  typedef vtkSmartPointer<vtkConeSource> vtkConeSourcePtr;
//  vtkConeSourcePtr coneSource = vtkConeSource::New();
//  coneSource->SetResolution(25);
//  coneSource->SetRadius(10);
//  coneSource->SetHeight(100);
//
//  coneSource->SetDirection(0,0,1);
//  double newCenter[3];
//  coneSource->GetCenter(newCenter);
//  newCenter[2] = newCenter[2] - coneSource->GetHeight()/2;
//  coneSource->SetCenter(newCenter);
//
//  //Cone rep visialization
//  ssc::MeshPtr coneSurface = ssc::MeshPtr(new ssc::Mesh("cone"));
//  coneSurface->setVtkPolyData(coneSource->GetOutput());
//  ssc::GeometricRepPtr coneSurfaceRep(ssc::GeometricRep::New("cone"));
//  coneSurfaceRep->setMesh(coneSurface);
//  view->addRep(coneSurfaceRep);
//
//  //print
//  coneSource->Update();
//  coneSource->GetOutput()->Print(std::cout);
//  //vtkPolyData* conePolyData = coneSource->GetOutput();
//
//  //vtkPolyData* surface = convert->GetOutput();
//
//  //test: Show surface
//  /*vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
//  mapper->SetInput(surface);
//  mapper->Update();
//  vtkActor* actor = vtkActor::New();
//  actor->SetMapper(mapper);
//  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
//  actor->SetVisibility(true);
//  actor->SetUserMatrix(matrix);
//
//  ViewManager::getInstance()->get3DView("View3D_1")->getRenderer()->AddActor(actor);
//  */
//
//  //Test save ITK object
//  ImageWriterType::Pointer writer = ImageWriterType::New();
//  writer->SetInput(data);
//  writer->SetFileName( outFileName.toLatin1() );
//  writer->Update();
//}
