/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/
#ifndef CXLOGMESSAGEFILTER_H
#define CXLOGMESSAGEFILTER_H

#include "cxResourceExport.h"
#include "cxReporter.h"

#include <vector>
#include <QPointer>

#include "cxEnumConverter.h"

namespace cx
{

static LOG_SEVERITY level2severity(MESSAGE_LEVEL level)
{
	switch (level)
	{
	// level 1
	case mlCERR :
	case mlERROR : return msERROR;

	// level 2
	case mlSUCCESS :
	case mlWARNING : return msWARNING;

	// level 3
	case mlINFO : return msINFO;

	// level 4
	case mlCOUT :
	case mlDEBUG : return msDEBUG;

	default: return msCOUNT;
	}
}

typedef boost::shared_ptr<class MessageFilter> MessageFilterPtr;

class MessageFilter
{
public:
	~MessageFilter() {}
	virtual bool operator()(const Message& msg) const = 0;
	virtual MessageFilterPtr clone() = 0;

};

class MessageFilterConsole : public MessageFilter
{
public:
	virtual bool operator()(const Message& msg) const
	{
		if (!isActiveChannel(msg))
			return false;
		if (!isActiveSeverity(msg))
			return false;
		return true;
	}

	virtual MessageFilterPtr clone()
	{
		return MessageFilterPtr(new MessageFilterConsole(*this));
	}


	bool isActiveSeverity(const Message& msg) const
	{
		LOG_SEVERITY severity = level2severity(msg.getMessageLevel());
		return severity <= mLowestSeverity;
	}

	bool isActiveChannel(const Message& msg) const
	{
		if (mChannel == "all")
			return true;
		if (msg.mChannel == mChannel)
			return true;
		return false;
	}

	void setActiveChannel(QString uid)
	{
		mChannel = uid;
	}

	void clearSeverity()
	{
		mLowestSeverity = msERROR;
	}
	void activateSeverity(LOG_SEVERITY severity)
	{
		mLowestSeverity = std::max(mLowestSeverity, severity);
	}
	void setLowestSeverity(LOG_SEVERITY severity)
	{
		mLowestSeverity = severity;
	}
	LOG_SEVERITY getLowestSeverity() const
	{
		return mLowestSeverity;
	}

private:
	QString mChannel;
	LOG_SEVERITY mLowestSeverity;
};


} // namespace cx

#endif // CXLOGMESSAGEFILTER_H
