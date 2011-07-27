/*
 * cxAngleMetricRep.h
 *
 *  Created on: Jul 27, 2011
 *      Author: christiana
 */

#ifndef CXANGLEMETRICREP_H_
#define CXANGLEMETRICREP_H_

#include <sscRepImpl.h>
#include "sscGraphicalPrimitives.h"
#include "cxAngleMetric.h"
#include "vtkForwardDeclarations.h"

#include "vtkTextActor.h"
typedef vtkSmartPointer<vtkTextActor> vtkTextActorPtr;

namespace cx
{

typedef boost::shared_ptr<class AngleMetricRep> AngleMetricRepPtr;

/**Rep for visualizing a DistanceMetric.
 *
 */
class AngleMetricRep : public ssc::RepImpl
{
  Q_OBJECT
public:
  static AngleMetricRepPtr New(const QString& uid, const QString& name=""); ///constructor
  virtual ~AngleMetricRep() {}

  void setMetric(AngleMetricPtr point);
  virtual QString getType() const { return "cx::AngleMetricRep"; }

  void scaleText();

protected:
  virtual void addRepActorsToViewRenderer(ssc::View* view);
  virtual void removeRepActorsFromViewRenderer(ssc::View* view);

protected slots:
  void changedSlot();

private:
  AngleMetricRep(const QString& uid, const QString& name="");
  AngleMetricRep(); ///< not implemented

  ssc::GraphicalLine3DPtr mLine0;
  ssc::GraphicalLine3DPtr mLine1;
  AngleMetricPtr mMetric;
  ssc::View* mView;
  ssc::Vector3D mColor;
  typedef vtkSmartPointer<class AngleMetricCallback> AngleMetricCallbackPtr;
  AngleMetricCallbackPtr mMetricCallback;

//  vtkTextActorPtr mTextActor;
  vtkVectorTextPtr mText;
  vtkFollowerPtr mFollower;
};


}

#endif /* CXANGLEMETRICREP_H_ */
