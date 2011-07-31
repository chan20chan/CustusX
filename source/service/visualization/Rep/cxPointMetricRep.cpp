/*
 * cxPointMetricRep.cpp
 *
 *  Created on: Jul 5, 2011
 *      Author: christiana
 */

#include <cxPointMetricRep.h>
#include "sscView.h"
#include "boost/bind.hpp"

namespace cx
{

PointMetricRepPtr PointMetricRep::New(const QString& uid, const QString& name)
{
	PointMetricRepPtr retval(new PointMetricRep(uid,name));
	return retval;
}

PointMetricRep::PointMetricRep(const QString& uid, const QString& name) :
		DataMetricRep(uid,name),
		mView(NULL)
//		mSphereRadius(1),
//		mShowLabel(false),
//		mColor(ssc::Vector3D(1,0,0))
{
  mViewportListener.reset(new ssc::ViewportListener);
	mViewportListener->setCallback(boost::bind(&PointMetricRep::rescale, this));
}

//void PointMetricRep::setShowLabel(bool on)
//{
//  mShowLabel = on;
//  this->changedSlot();
//}

void PointMetricRep::setPointMetric(PointMetricPtr point)
{
	if (mMetric)
		disconnect(mMetric.get(), SIGNAL(transformChanged()), this, SLOT(changedSlot()));

	mMetric = point;

	if (mMetric)
		connect(mMetric.get(), SIGNAL(transformChanged()), this, SLOT(changedSlot()));

	mGraphicalPoint.reset();
	this->changedSlot();
}

void PointMetricRep::addRepActorsToViewRenderer(ssc::View* view)
{
	mView = view;
	mGraphicalPoint.reset();
	mText.reset();
	mViewportListener->startListen(mView->getRenderer());
	this->changedSlot();
}

void PointMetricRep::removeRepActorsFromViewRenderer(ssc::View* view)
{
	mView = NULL;
	mGraphicalPoint.reset();
  mText.reset();
	mViewportListener->stopListen();
}

//void PointMetricRep::setSphereRadius(double radius)
//{
//  mSphereRadius = radius;
//  this->changedSlot();
//}

void PointMetricRep::changedSlot()
{
  if (!mMetric)
    return;

	if (!mGraphicalPoint && mView && mMetric)
		mGraphicalPoint.reset(new ssc::GraphicalPoint3D(mView->getRenderer()));

	if (!mGraphicalPoint)
		return;

	ssc::Transform3D rM0 = ssc::SpaceHelpers::get_toMfrom(mMetric->getFrame(), ssc::CoordinateSystem(ssc::csREF));
	ssc::Vector3D p0_r = rM0.coord(mMetric->getCoordinate());

	mGraphicalPoint->setValue(p0_r);
	mGraphicalPoint->setRadius(mGraphicsSize);
	mGraphicalPoint->setColor(mColor);

  if (!mShowLabel)
    mText.reset();
  if (!mText && mShowLabel)
    mText.reset(new ssc::FollowerText3D(mView->getRenderer()));
  if (mText)
  {
    mText->setColor(mColor);
    mText->setText(mMetric->getName());
    mText->setPosition(p0_r);
    mText->setSizeInNormalizedViewport(true, mLabelSize/100);
  }

  this->rescale();
}

/**Note: Internal method!
 *
 * Scale the text to be a constant fraction of the viewport height
 * Called from a vtk camera observer
 *
 */
void PointMetricRep::rescale()
{
  if (!mGraphicalPoint)
    return;

	double size = mViewportListener->getVpnZoom();
//  double sphereSize = 0.007/size;
  double sphereSize = mGraphicsSize/100/size;
  mGraphicalPoint->setRadius(sphereSize);
}

}
