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


#include "cxOpenIGTLinkGuiExtenderService.h"
/*
#include "cxNetworkConnection.h"
#include "cxNetworkConnectionWidget.h"
#include "cxNetworkDataTransferWidget.h"
#include "cxNetworkServiceImpl.h"
#include "cxNetworkDataTransfer.h"
#include "cxNetworkConnectionsWidget.h"
*/

#include "qIGTLIOClientWidget.h"
#include "qIGTLIOLogicController.h"

namespace cx
{
OpenIGTLink3GuiExtenderService::OpenIGTLink3GuiExtenderService(ctkPluginContext *context, vtkIGTLIOLogicPointer logic)
{
	mContext = context;
	mLogic = logic;

}

OpenIGTLink3GuiExtenderService::~OpenIGTLink3GuiExtenderService()
{
}

std::vector<GUIExtenderService::CategorizedWidget> OpenIGTLink3GuiExtenderService::createWidgets() const
{
	qIGTLIOLogicController* logicController = new qIGTLIOLogicController();
	logicController->setLogic(mLogic);

	qIGTLIOClientWidget* widget = new qIGTLIOClientWidget();
	widget->setWindowTitle("OpenIGTLink3");
	widget->setObjectName("Object_OpenIGTLink_3");
	widget->setLogic(mLogic);

	std::vector<CategorizedWidget> retval;
	retval.push_back(GUIExtenderService::CategorizedWidget( widget, "OpenIGTLink"));
	return retval;
}
}//namespace cx
