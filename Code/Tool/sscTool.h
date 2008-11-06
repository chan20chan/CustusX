#ifndef SSCTOOL_H_
#define SSCTOOL_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <vtkSmartPointer.h>
typedef vtkSmartPointer<class vtkMatrix4x4> vtkMatrix4x4Ptr;
typedef vtkSmartPointer<class vtkPolyData> vtkPolyDataPtr;
#include "sscTransform3D.h"

namespace ssc
{
/**Interface to a tool,
 * i.e. a pointer, US probe or similar.
 *
 * The tool position is defined in its own space (as given
 * by the tool's transform) as follows:
 *  - the origin is the tool tip
 * 	- the z axis points in the acting direction (us probe ray dir or pointing dir).
 *  - the y axis points to the left side of the tool.
 */
class Tool : public QObject
{
	Q_OBJECT
public:
	Tool() :
		mUid(""),
		mName("")
	{};
	~Tool(){};

	/**Enumerates the general type of tool.
	 */
	enum Type
	{
		TOOL_NONE,
		TOOL_REFERENCE,
		TOOL_POINTER,
		TOOL_US_PROBE
	};
	virtual Type getType() const = 0;
	/**\return a file containing a graphical description of the tool,
	 * if any. The file format is given by the file extension, for example
	 * usprobe_12L.stl for the SolidWorks format.
	 * \sa getGraphicsPolyData().
	 */
	virtual std::string getGraphicsFileName() const = 0;
	/**Get a pointer to the tools graphical data in the form of vtkPolyData,
	 * if any. Either getGraphicsPolyData() or getGraphicsFileName() or both
	 * should return valid data. \sa getGraphicsFileName().
	 */
	virtual vtkPolyDataPtr getGraphicsPolyData() const = 0;
	/**Saves the tools internal buffers of transforms and timestamps to file.
	 */
	virtual void saveTransformsAndTimestamps() = 0;
	/**Which file to use when calling saveTransformsAndTimestamps().
	 */
	virtual void setTransformSaveFile(const std::string& filename) = 0;
	virtual Transform3DPtr get_prMt() const = 0; ///< \return transform from tool to patient ref space
	virtual bool getVisible() const = 0; ///< \return the visibility status of the tool
	virtual std::string getUid() const = 0; ///< \return an unique id for this instance
	virtual std::string getName() const = 0; ///< \return a descriptive name for this instance

signals:
	void toolTransformAndTimestamp(Transform3D matrix, double timestamp);
	void toolVisible(bool visible);

protected:
	std::string mUid;
	std::string mName;
};
typedef boost::shared_ptr<Tool> ToolPtr;
} // namespace ssc

#endif /*SSCTOOL_H_*/
