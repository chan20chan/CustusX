#ifndef CXLANDMARKREP_H_
#define CXLANDMARKREP_H_

#include "sscRepImpl.h"

#include <vector>
#include "sscImage.h"

/**
 * cxLandmarkRep.h
 *
 * \brief
 *
 * \date Dec 10, 2008
 * \author: Janne Beate Bakeng, SINTEF
 */
namespace ssc
{
class View;
}
typedef vtkSmartPointer<class vtkSphereSource> vtkSphereSourcePtr;
typedef vtkSmartPointer<class vtkPolyDataMapper> vtkPolyDataMapperPtr;
typedef vtkSmartPointer<class vtkActor> vtkActorPtr;
typedef vtkSmartPointer<class vtkFollower> vtkFollowerPtr;
typedef vtkSmartPointer<class vtkVectorText> vtkVectorTextPtr;
typedef std::pair<vtkVectorTextPtr,vtkFollowerPtr> vtkVectorTextFollowerPair;
namespace cx
{
class MessageManager;
typedef boost::shared_ptr<class LandmarkRep> LandmarkRepPtr;
typedef boost::weak_ptr<class LandmarkRep> LandmarkRepWeakPtr;

class LandmarkRep : public ssc::RepImpl
{
  Q_OBJECT
public:
  struct RGB
  {
    int R;
    int G;
    int B;
    RGB() : R(255), G(0), B(0)
    {};
  };

  static LandmarkRepPtr New(const std::string& uid, const std::string& name="");
  ~LandmarkRep();

  virtual std::string getType() const;

  int getNumberOfLandmarks() const; ///< returns the number of landmarks
  void setColor(RGB color); ///< sets the reps color
  void setTextScale(int& x, int& y,int& z); ///< default is (20,20,20)
  void showLandmarks(bool on); ///< turn on or off showing landmarks
  void setImage(ssc::ImagePtr image); ///< sets the image data should be retrieved from
  ssc::ImagePtr getImage() const; ///< returns a pointer to the image being used
  //TODO: REMOVE
  void removePermanentPoint(unsigned int idNumber); ///< sends out a signal requesting a point to be removed from the images internal landmarklist
  void requestRemovePermanentPoint(double x, double y, double z);

signals:
  void removePermanentPoint(double x, double y, double z); ///< the landmarkrep can signal that a point should be removed the (images) landmarklist

public slots:
  void addPermanentPointSlot(double x, double y, double z); ///< used tell the landmarkrep that a new point is added

protected:
  LandmarkRep(const std::string& uid, const std::string& name="");
  virtual void addRepActorsToViewRenderer(ssc::View* view);
  virtual void removeRepActorsFromViewRenderer(ssc::View* view);
  void addPoint(double& x, double& y, double& z, int numberInLine=0); ///< add a set of actors for the new point
  void internalUpdate(); ///< run after a point is removed to update the numbers of the textactors

  MessageManager&     mMessageManager;  ///< device for sending messages to the statusbar

  std::string     mType;          ///< description of this reps type
  RGB             mColor;         ///< the color of the landmark actors
  ssc::ImagePtr   mImage;         ///< the image which this rep is linked to
  bool            mShowLandmarks; ///< whether or not the actors should be showed in (all) views
  int             mTextScale[3];  ///< the textscale

  std::vector<vtkActorPtr>                mSkinPointActors;   ///< list of actors used to show where the point is on the skin
  std::vector<vtkVectorTextFollowerPair>  mTextFollowerActors; ///< list of numberactors with the text representing the number for easy updating

private:
  LandmarkRep(); ///< not implemented
};
}//namespace cx
#endif /* CXLANDMARKREP_H_ */
