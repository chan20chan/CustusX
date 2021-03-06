# =========================================================================
# This file is part of CustusX, an Image Guided Therapy Application.
#
# Copyright (c) SINTEF Department of Medical Technology.
# All rights reserved.
#
# CustusX is released under a BSD 3-Clause license.
#
# See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
# =========================================================================

###########################################################
#               cxPluginAlgorithm Tests
###########################################################

if(BUILD_TESTING)
    cx_add_class(CXTEST_SOURCES ${CXTEST_SOURCES}
        cxtestCenterlineRegistration.cpp
        cxtestExportDummyClassForLinkingOnWindowsInLibWithoutExportedClass.cpp
    )
    set(CXTEST_SOURCES_TO_MOC
    )

    qt5_wrap_cpp(CXTEST_SOURCES_TO_MOC ${CXTEST_SOURCES_TO_MOC})
    add_library(cxtest_org_custusx_registration_method_centerline ${CXTEST_SOURCES} ${CXTEST_SOURCES_TO_MOC})
    include(GenerateExportHeader)
    generate_export_header(cxtest_org_custusx_registration_method_centerline)
    target_include_directories(cxtest_org_custusx_registration_method_centerline
        PUBLIC
        .
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(cxtest_org_custusx_registration_method_centerline
        PRIVATE
        org_custusx_registration_method_centerline
        cxLogicManager
        cxtestUtilities
        cxCatch
        cxResource
        cxtestResource
        cxtest_org_custusx_core_patientmodel
    )
    cx_add_tests_to_catch(cxtest_org_custusx_registration_method_centerline)

endif(BUILD_TESTING)
