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

#include "cxNetworkHandler.h"

#include <QTimer>
#include "vtkIGTLIOLogic.h"
#include "vtkIGTLIOImageDevice.h"
#include "cxLogger.h"
#include "igtlImageConverter.h"

namespace cx
{

NetworkHandler::NetworkHandler(vtkIGTLIOLogicPointer logic) :
	mTimer(new QTimer(this))
{
	mLogic = logic;
	foreach(int eventId, QList<int>()
			<< vtkIGTLIOLogic::ConnectionAddedEvent
			<< vtkIGTLIOLogic::ConnectionAboutToBeRemovedEvent
			)
	{
		CX_LOG_DEBUG() << "connecting to the vtkIGTIOLogic events";
		qvtkReconnect(NULL, mLogic, eventId,
					  this, SLOT(onConnectionEvent(vtkObject*, void*, unsigned long, void*)));
	}
	foreach(int eventId, QList<int>()
			<< vtkIGTLIOLogic::NewDeviceEvent
			<< vtkIGTLIOLogic::RemovedDeviceEvent
			)
	{
		CX_LOG_DEBUG() << "connecting onDeviceAddedOrRemoved to the vtkIGTIOLogic events";
		qvtkReconnect(NULL, mLogic, eventId,
					this, SLOT(onDeviceAddedOrRemoved(vtkObject*, void*, unsigned long, void*)));
	}


	connect(mTimer, SIGNAL(timeout()), this, SLOT(periodicProcess()));
	mTimer->start(5);
}

NetworkHandler::~NetworkHandler()
{
	mTimer->stop();
}


void NetworkHandler::onDeviceModified(vtkObject* caller, void* device, unsigned long event , void*)
{
	CX_LOG_DEBUG() << "NetworkHandler::onDeviceModified";
	vtkSmartPointer<vtkIGTLIODevice> receivedDevice(reinterpret_cast<vtkIGTLIODevice*>(caller));
	igtl::BaseConverter::HeaderData header = receivedDevice->GetHeader();
	std::string device_type = receivedDevice->GetDeviceType();
	if(device_type == igtl::ImageConverter::GetIGTLTypeName())//Only process image messages for now
	{
		vtkSmartPointer<vtkIGTLIOImageDevice> imageDevice = vtkIGTLIOImageDevice::SafeDownCast(receivedDevice);
		igtl::ImageConverter::ContentData content = imageDevice->GetContent();
		ImagePtr cximage = ImagePtr(new Image("uid", content.image));
		emit image(cximage);
	}

}

void NetworkHandler::onConnectionEvent(vtkObject* caller, void* connector, unsigned long event , void*)
{
	if (event==vtkIGTLIOLogic::ConnectionAddedEvent)
	{
		CX_LOG_DEBUG("") << "on add connected event";
		//vtkIGTLIOConnector* c = static_cast<vtkIGTLIOConnector*>(connector);
		//this->ReconnectConnector(NULL, c);
		//this->resetModel();
	}
	if (event==vtkIGTLIOLogic::ConnectionAboutToBeRemovedEvent)
	{
		CX_LOG_DEBUG() << "on remove connected event";
		//vtkIGTLIOConnector* c = static_cast<vtkIGTLIOConnector*>(connector);
		//this->ReconnectConnector(c, NULL);
		//this->resetModel();
	}
}

void NetworkHandler::onDeviceAddedOrRemoved(vtkObject*caller, void* connector, unsigned long event, void* callData)
{
	CX_LOG_DEBUG("") << "NetworkHandler::onDeviceAddedOrRemoved";
	if (event==vtkIGTLIOLogic::NewDeviceEvent)
	{
		CX_LOG_DEBUG("") << "on add devices event";
		//vtkIGTLIOConnector* c = static_cast<vtkIGTLIOConnector*>(connector);

//		vtkIGTLIODevicePointer device = mLogic->GetDevice(i);
		vtkIGTLIODevicePointer device(reinterpret_cast<vtkIGTLIODevice*>(callData));

		if(device)
		{
			CX_LOG_DEBUG() << "Found a device";
			qvtkReconnect(NULL, device, vtkIGTLIODevice::ModifiedEvent, this, SLOT(onDeviceModified(vtkObject*, void*, unsigned long, void*)));
		}
		else
			CX_LOG_ERROR() << "Not a device";
	}
	if (event==vtkIGTLIOLogic::RemovedDeviceEvent)
	{
		CX_LOG_DEBUG() << "on remove device event";
		//vtkIGTLIOConnector* c = static_cast<vtkIGTLIOConnector*>(connector);

	}
}

void NetworkHandler::periodicProcess()
{
	CX_LOG_DEBUG() << "NetworkHandler::periodicProcess()";
	mLogic->PeriodicProcess();
}

} // namespace cx
