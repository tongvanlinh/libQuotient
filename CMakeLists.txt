cmake_minimum_required(VERSION 3.26)
if (POLICY CMP0092)
cmake_policy(SET CMP0092 NEW)
endif()

set(API_VERSION "0.9")
project(Quotient VERSION "${API_VERSION}.2" LANGUAGES CXX)
set(PRE_STAGE "")
string(JOIN ~ FULL_VERSION ${PROJECT_VERSION} ${PRE_STAGE})

message(STATUS)
message(STATUS "Configuring ${PROJECT_NAME} ${FULL_VERSION} ==>")

include(FeatureSummary)
include(CTest)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
include(CheckCXXCompilerFlag)
if (MSVC)
    add_compile_options(/EHsc /W4
        /wd4100 /wd4127 /wd4242 /wd4244 /wd4245 /wd4267 /wd4365 /wd4456 /wd4459
        /wd4464 /wd4505 /wd4514 /wd4571 /wd4619 /wd4623 /wd4625 /wd4626 /wd4706
        /wd4710 /wd4774 /wd4820 /wd4946 /wd5026 /wd5027 /wd4702)
else()
    foreach (FLAG all pedantic extra error=return-type) # Switch these on
        CHECK_CXX_COMPILER_FLAG("-W${FLAG}" W${FLAG}_SUPPORTED)
        if (W${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "W(no-)?${FLAG}($| )")
            add_compile_options(-W${FLAG})
        endif ()
    endforeach ()
    foreach (FLAG unused-parameter gnu-zero-variadic-macro-arguments
                  subobject-linkage) # Switch these off
        CHECK_CXX_COMPILER_FLAG("-Wno-${FLAG}" Wno-${FLAG}_SUPPORTED)
        if (Wno-${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "W(no-)?${FLAG}($| )")
            add_compile_options(-Wno-${FLAG})
        endif()
    endforeach ()
    # disable exceptions by default, not used in most parts here and saves about 10% binary size
    add_compile_options(-fno-exceptions)
endif()

add_compile_definitions(
    QT_NO_JAVA_STYLE_ITERATORS QT_NO_URL_CAST_FROM_STRING QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_ASCII QT_STRICT_ITERATORS QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT QT_NO_FOREACH)

set_directory_properties(PROPERTIES
    CXX_STANDARD 23
    CXX_EXTENSIONS OFF
    VISIBILITY_INLINES_HIDDEN ON
    CXX_VISIBILITY_PRESET hidden
)

set(QtMinVersion "6.4")
string(REGEX REPLACE "^(.).*" "Qt\\1" Qt ${QtMinVersion}) # makes "Qt<MajorVersionDigit>"
set(QUOTIENT_LIB_NAME ${PROJECT_NAME}${Qt})

if (WIN32)
    if (NOT CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR ".")
        set(CMakeFilesLocation "cmake")
    else()
        set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
    endif()

    if (NOT CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ".")
    endif()

    if (NOT CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif()
else()
    include(GNUInstallDirs)
    set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
endif(WIN32)
set(${PROJECT_NAME}_INSTALL_INCLUDEDIR
        "${CMAKE_INSTALL_INCLUDEDIR}/" CACHE PATH
        "directory to install ${PROJECT_NAME} include files to")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(${Qt} ${QtMinVersion} REQUIRED Core Network Gui Test Sql QmlIntegration)
get_filename_component(Qt_Prefix "${${Qt}_DIR}/../../../.." ABSOLUTE)

find_package(${Qt}Keychain REQUIRED)

find_package(Olm 3.2.5 REQUIRED)
set_package_properties(Olm PROPERTIES
    DESCRIPTION "Implementation of the Olm and Megolm cryptographic ratchets"
    URL "https://gitlab.matrix.org/matrix-org/olm"
    TYPE REQUIRED
)

find_package(OpenSSL 1.1.0 REQUIRED)
set_package_properties(OpenSSL PROPERTIES
    DESCRIPTION "Open source SSL and TLS implementation and cryptographic library"
    URL "https://www.openssl.org/"
    TYPE REQUIRED
)

add_library(${QUOTIENT_LIB_NAME})

add_subdirectory(Quotient)

target_sources(${QUOTIENT_LIB_NAME} PRIVATE libquotientemojis.qrc)

# Set BUILDING_SHARED_QUOTIENT if building as a shared library
target_compile_definitions(${QUOTIENT_LIB_NAME} PRIVATE
    $<$<STREQUAL:$<TARGET_PROPERTY:${QUOTIENT_LIB_NAME},TYPE>,SHARED_LIBRARY>:BUILDING_SHARED_QUOTIENT>)
# Set QUOTIENT_STATIC in a static library setting
target_compile_definitions(${QUOTIENT_LIB_NAME} PUBLIC
    $<$<STREQUAL:$<TARGET_PROPERTY:${QUOTIENT_LIB_NAME},TYPE>,STATIC_LIBRARY>:QUOTIENT_STATIC>)
target_compile_definitions(${QUOTIENT_LIB_NAME} PUBLIC ${PROJECT_NAME}_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ${PROJECT_NAME}_VERSION_MINOR=${PROJECT_VERSION_MINOR} ${PROJECT_NAME}_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ${PROJECT_NAME}_VERSION_STRING=\"${PROJECT_VERSION}\" ${PROJECT_NAME}_PRE_STAGE=\"${PRE_STAGE}\"
    ${PROJECT_NAME}_E2EE_ENABLED)
set_target_properties(${QUOTIENT_LIB_NAME} PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION ${API_VERSION}
    INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${API_VERSION}
)
set_property(TARGET ${QUOTIENT_LIB_NAME} APPEND PROPERTY
             COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

target_compile_features(${QUOTIENT_LIB_NAME} PUBLIC cxx_std_23)
if (MSVC)
    target_compile_options(${QUOTIENT_LIB_NAME} PUBLIC /Zc:preprocessor)
endif()

# Don't use PCH w/GCC (https://bugzilla.redhat.com/show_bug.cgi?id=1721553#c34)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_precompile_headers(${QUOTIENT_LIB_NAME} PRIVATE Quotient/converters.h)
endif ()

target_include_directories(${QUOTIENT_LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${QUOTIENT_FORCE_NAMESPACED_INCLUDES}>>:${CMAKE_CURRENT_SOURCE_DIR}/Quotient>>
)

target_link_libraries(${QUOTIENT_LIB_NAME}
    PUBLIC ${Qt}::Core ${Qt}::Network ${Qt}::Gui qt${${Qt}Core_VERSION_MAJOR}keychain Olm::Olm ${Qt}::Sql
    PRIVATE OpenSSL::Crypto ${Qt}::CorePrivate)

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}.pc @ONLY NEWLINE_STYLE UNIX)

# Configure testing

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(quotest)
    add_subdirectory(autotests)
endif()

# Configure installation

install(TARGETS ${QUOTIENT_LIB_NAME} EXPORT ${QUOTIENT_LIB_NAME}Targets
    RUNTIME LIBRARY ARCHIVE FILE_SET HEADERS)

include(CMakePackageConfigHelpers)
# NB: SameMajorVersion doesn't really work yet, as we're within 0.x trail.
# Maybe consider jumping the gun and releasing 1.0, as semver advises?
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

export(PACKAGE ${QUOTIENT_LIB_NAME})
export(EXPORT ${QUOTIENT_LIB_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Targets.cmake")

# make sure BUILD_SHARED_LIBS is always defined for generating the QuotientConfig.cmake file
if (NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()
configure_file(cmake/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Config.cmake"
    @ONLY
)

install(EXPORT ${QUOTIENT_LIB_NAME}Targets
        FILE ${QUOTIENT_LIB_NAME}Targets.cmake DESTINATION ${CMakeFilesLocation})

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}ConfigVersion.cmake"
    DESTINATION ${CMakeFilesLocation}
)
install(EXPORT_ANDROID_MK ${QUOTIENT_LIB_NAME}Targets DESTINATION ${CMAKE_INSTALL_DATADIR}/ndk-modules)

if (WIN32)
    install(FILES mime/packages/freedesktop.org.xml DESTINATION mime/packages)
endif (WIN32)

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

message(STATUS)
message(STATUS "== libQuotient ${FULL_VERSION} configuration summary ==")
message(STATUS)
if (CMAKE_BUILD_TYPE)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Header files install prefix: ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_INSTALL_INCLUDEDIR}")
message(STATUS "Using Qt ${${Qt}_VERSION} at ${Qt_Prefix}")
message(STATUS "Using QtKeychain ${${Qt}Keychain_VERSION} at ${${Qt}Keychain_DIR}")
message(STATUS "Using libOlm ${Olm_VERSION} at ${Olm_DIR}")
message(STATUS "Using OpenSSL libcrypto ${OPENSSL_VERSION} at ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS)
feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES
                FATAL_ON_MISSING_REQUIRED_PACKAGES)

message(STATUS "<== End of libQuotient configuration")
