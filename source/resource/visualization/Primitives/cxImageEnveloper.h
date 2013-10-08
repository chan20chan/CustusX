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

#ifndef CXMULTIVOLUMEBOUNDINGBOX_H
#define CXMULTIVOLUMEBOUNDINGBOX_H

#include "cxForwardDeclarations.h"
#include "sscVector3D.h"
#include "sscTransform3D.h"
#include <vector>
#include "cxImageParameters.h"

namespace cx
{

typedef boost::shared_ptr<class ImageEnveloper> ImageEnveloperPtr;

/**
 * Generate a total bounding volume from a set of volumes.
 *
 * \ingroup cxServiceVisualizationRep
 * \date 3 Oct 2013
 * \author Christian Askeland, SINTEF
 * \author Ole Vegard Solberg, SINTEF
 */
class ImageEnveloper
{
public:
	static ImageEnveloperPtr create();
	ImageEnveloper() {}
	virtual ~ImageEnveloper() {}

	virtual void setImages(std::vector<ImagePtr> images);
	//virtual Box getBox() const;
	virtual ImagePtr getEnvelopingImage(long maxVoxels = 0);

private:
	std::vector<ImagePtr> mImages;

	ImageParameters reduceToNumberOfVoxels(ImageParameters box, long maxVoxels);
	ImageParameters createEnvelopeParametersFromImage(ImagePtr img);
	ImageParameters selectParametersWithSmallestExtent(ImageParameters a, ImageParameters b);
	ImageParameters selectParametersWithFewestVoxels(ImageParameters a, ImageParameters b);
	ImagePtr createEnvelopeFromParameters(ImageParameters box);
	Eigen::Array3d getMinimumSpacingFromAllImages(Transform3D qMr);
	Eigen::Array3d getTransformedSpacing(Eigen::Array3d spacing, Transform3D qMd);
};

} // namespace cx

#endif // CXMULTIVOLUMEBOUNDINGBOX_H
