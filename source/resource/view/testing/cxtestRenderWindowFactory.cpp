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


#include "catch.hpp"
#include "cxRenderWindowFactory.h"
#include "cxSharedOpenGLContext.h"
#include <vtkRenderWindow.h>
#include <vtkOpenGLRenderWindow.h>
#include <QString>


namespace
{

cx::SharedOpenGLContextPtr initSharedOpenGLContext(vtkRenderWindowPtr renderWindow)
{
	vtkOpenGLRenderWindowPtr opengl_renderwindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get());
	REQUIRE(opengl_renderwindow);
	REQUIRE(cx::SharedOpenGLContext::isValid(opengl_renderwindow));
	cx::SharedOpenGLContextPtr sharedOpenGLContext = cx::SharedOpenGLContextPtr(new cx::SharedOpenGLContext(opengl_renderwindow));
	return sharedOpenGLContext;
}
}//

namespace cxtest {

TEST_CASE("RenderWindowFactory init", "[opengl][resource][visualization][unit]")
{
    cx::RenderWindowFactoryPtr renderWindowFactory = cx::RenderWindowFactoryPtr(new cx::RenderWindowFactory());
    REQUIRE(renderWindowFactory);
}

TEST_CASE("RenderWindowFactory init shared render window", "[opengl][resource][visualization][unit]")
{
    cx::RenderWindowFactoryPtr renderWindowFactory = cx::RenderWindowFactoryPtr(new cx::RenderWindowFactory());
    vtkRenderWindowPtr renderWindow = renderWindowFactory->getSharedRenderWindow();
    REQUIRE(initSharedOpenGLContext(renderWindow));
    REQUIRE(renderWindow);
}

TEST_CASE("RenderWindowFactory get render window", "[opengl][resource][visualization][unit]")
{
    cx::RenderWindowFactoryPtr renderWindowFactory = cx::RenderWindowFactoryPtr(new cx::RenderWindowFactory());
    QString uid = "TestWindowUid1";
    vtkRenderWindowPtr renderWindow1 = renderWindowFactory->getRenderWindow(uid);
    REQUIRE(renderWindow1);

    vtkRenderWindowPtr renderWindow1Copy = renderWindowFactory->getRenderWindow(uid);

    REQUIRE (renderWindow1 == renderWindow1Copy);
}

//TEST_CASE("RenderWindowFactory render", "[opengl][resource][visualization][unit]")
//{
//	vtkRenderWindowPtr renderWindow = vtkRenderWindowPtr::New();
//	REQUIRE(renderWindow);
//	renderWindow->Render();
//}

TEST_CASE("RenderWindowFactory render in render window", "[opengl][resource][visualization][unit]")
{
    cx::RenderWindowFactoryPtr renderWindowFactory = cx::RenderWindowFactoryPtr(new cx::RenderWindowFactory());
    QString uid = "TestWindowUid1";
    vtkRenderWindowPtr renderWindow1 = renderWindowFactory->getRenderWindow(uid, false);
    REQUIRE(renderWindow1);
    renderWindow1->Render();
}

//This test requires only one OpenGL context, so it have to run by itself (run as integration)
TEST_CASE("RenderWindowFactory render a single renderWindow", "[opengl][resource][visualization][integration]")
{
	cx::RenderWindowFactoryPtr renderWindowFactory = cx::RenderWindowFactoryPtr(new cx::RenderWindowFactory());
	QString uid("cx_shared_context");
	REQUIRE(renderWindowFactory->renderWindowExists(uid));
	vtkRenderWindowPtr renderWindow = renderWindowFactory->getRenderWindow(uid);
//    REQUIRE(initSharedOpenGLContext(renderWindow));
	REQUIRE(renderWindow);
	renderWindow->Render();

	vtkOpenGLRenderWindowPtr opengl_renderwindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get());
	REQUIRE(opengl_renderwindow);
	REQUIRE(cx::SharedOpenGLContext::isValid(opengl_renderwindow));


	vtkRenderWindowPtr sharedRenderWindow = renderWindowFactory->getSharedRenderWindow();
	REQUIRE(sharedRenderWindow == renderWindow);

	QString testUid("testUid");
	REQUIRE_FALSE(renderWindowFactory->renderWindowExists(testUid));
	vtkRenderWindowPtr renderWindow2 = renderWindowFactory->getRenderWindow(testUid);
	REQUIRE(renderWindow2);

	vtkOpenGLRenderWindowPtr opengl_renderwindow2 = vtkOpenGLRenderWindow::SafeDownCast(renderWindow2.Get());
	REQUIRE(opengl_renderwindow2);
	REQUIRE(cx::SharedOpenGLContext::isValid(opengl_renderwindow2));

	REQUIRE(renderWindowFactory->renderWindowExists(testUid));
	for(int i = 0; i < 10; ++i)
	{
		renderWindow2->Render();
		sharedRenderWindow->Render();
	}

}

}//cxtest