# SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

include(CMakeParseArguments)

add_custom_target(autotests ALL)

add_definitions(-DDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" )

function(QUOTIENT_ADD_TEST)
    cmake_parse_arguments(ARG "" "NAME" "" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_NAME}.cpp testutils.h testutils.cpp)
    target_link_libraries(${ARG_NAME} ${Qt}::Core ${Qt}::Test ${QUOTIENT_LIB_NAME})
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    add_dependencies(autotests ${ARG_NAME})
    set_target_properties(${ARG_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_EXTENSIONS OFF
        VISIBILITY_INLINES_HIDDEN ON
        CXX_VISIBILITY_PRESET hidden
    )
endfunction()

quotient_add_test(NAME callcandidateseventtest)
quotient_add_test(NAME utiltests)
quotient_add_test(NAME testolmaccount)
quotient_add_test(NAME testgroupsession)
quotient_add_test(NAME testolmsession)
if (NOT MSVC)
    target_compile_options(testolmsession PRIVATE -fexceptions)
endif()
quotient_add_test(NAME testolmutility)
quotient_add_test(NAME testcryptoutils)
quotient_add_test(NAME testkeyverification)
quotient_add_test(NAME testcrosssigning)
quotient_add_test(NAME testkeyimport)
quotient_add_test(NAME testsettings)
quotient_add_test(NAME testthread)

add_test(NAME testqmltypes COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/testqmltypes.sh ${KDE_INSTALL_FULL_LIBDIR}/qml/io/github/quotient_im/libquotient)
