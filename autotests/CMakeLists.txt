# SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

include(CMakeParseArguments)

add_custom_target(autotests ALL)

add_definitions(-DDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" )

function(QUOTIENT_ADD_TEST)
    cmake_parse_arguments(ARG "" "NAME" "" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_NAME}.cpp testutils.h testutils.cpp)
    target_include_directories(${ARG_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Quotient)
    target_link_libraries(${ARG_NAME} PRIVATE ${Qt}::Core ${Qt}::Test ${QUOTIENT_LIB_NAME})
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    add_dependencies(autotests ${ARG_NAME})
endfunction()

quotient_add_test(NAME callevents_tests)
quotient_add_test(NAME util_tests)
quotient_add_test(NAME qolmaccount_test)
quotient_add_test(NAME groupsession_test)
quotient_add_test(NAME qolmsession_test)
quotient_add_test(NAME qolmutility_test)
quotient_add_test(NAME cryptoutils_test)
quotient_add_test(NAME keyverification_test)
quotient_add_test(NAME cross_signing_test)
quotient_add_test(NAME keyimport_test)
quotient_add_test(NAME settings_test)
quotient_add_test(NAME thread_test)
