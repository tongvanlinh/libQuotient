target_sources(${QUOTIENT_LIB_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ..
    FILES
        quotient_common.h
        quotient_export.h
        function_traits.h
        omittable.h
        expected.h
        networkaccessmanager.h
        connectiondata.h
        connection.h
        connection_p.h
        ssosession.h
        logging_categories_p.h
        room.h
        roomstateview.h
        user.h
        roommember.h
        avatar.h
        uri.h
        uriresolver.h
        eventstats.h
        syncdata.h
        settings.h
        networksettings.h
        converters.h
        util.h
        ranges_extras.h
        eventitem.h
        accountregistry.h
        mxcreply.h
        events/event.h
        events/roomevent.h
        events/stateevent.h
        events/single_key_value.h
        events/simplestateevents.h
        events/eventcontent.h
        events/eventrelation.h
        events/roomcreateevent.h
        events/roomtombstoneevent.h
        events/roommessageevent.h
        events/roommemberevent.h
        events/roomcanonicalaliasevent.h
        events/roomavatarevent.h
        events/roompowerlevelsevent.h
        events/typingevent.h
        events/accountdataevents.h
        events/receiptevent.h
        events/reactionevent.h
        events/callevents.h
        events/directchatevent.h
        events/encryptionevent.h
        events/encryptedevent.h
        events/roomkeyevent.h
        events/stickerevent.h
        events/filesourceinfo.h
        events/redactionevent.h
        jobs/requestdata.h
        jobs/basejob.h
        jobs/jobhandle.h
        jobs/syncjob.h
        jobs/mediathumbnailjob.h
        jobs/downloadfilejob.h
        database.h
        connectionencryptiondata_p.h
        keyverificationsession.h
        e2ee/e2ee_common.h
        e2ee/qolmaccount.h
        e2ee/qolmsession.h
        e2ee/qolminboundsession.h
        e2ee/qolmoutboundsession.h
        e2ee/qolmutility.h
        e2ee/qolmsession.h
        e2ee/qolmmessage.h
        e2ee/cryptoutils.h
        e2ee/sssshandler.h
        events/keyverificationevent.h
        keyimport.h
        qt_connection_util.h
        thread.h
    PRIVATE
        function_traits.cpp
        networkaccessmanager.cpp
        connectiondata.cpp
        connection.cpp
        ssosession.cpp
        room.cpp
        roomstateview.cpp
        user.cpp
        roommember.cpp
        avatar.cpp
        uri.cpp
        uriresolver.cpp
        eventstats.cpp
        syncdata.cpp
        settings.cpp
        networksettings.cpp
        converters.cpp
        util.cpp
        eventitem.cpp
        accountregistry.cpp
        mxcreply.cpp
        events/event.cpp
        events/roomevent.cpp
        events/stateevent.cpp
        events/eventcontent.cpp
        events/eventrelation.cpp
        events/roomcreateevent.cpp
        events/roomtombstoneevent.cpp
        events/roommessageevent.cpp
        events/roommemberevent.cpp
        events/roompowerlevelsevent.cpp
        events/receiptevent.cpp
        events/callevents.cpp
        events/directchatevent.cpp
        events/encryptionevent.cpp
        events/encryptedevent.cpp
        events/filesourceinfo.cpp
        jobs/requestdata.cpp
        jobs/basejob.cpp
        jobs/syncjob.cpp
        jobs/mediathumbnailjob.cpp
        jobs/downloadfilejob.cpp
        database.cpp
        connectionencryptiondata_p.cpp
        keyverificationsession.cpp
        e2ee/e2ee_common.cpp
        e2ee/qolmaccount.cpp
        e2ee/qolmsession.cpp
        e2ee/qolminboundsession.cpp
        e2ee/qolmoutboundsession.cpp
        e2ee/qolmutility.cpp
        e2ee/qolmsession.cpp
        e2ee/qolmmessage.cpp
        e2ee/cryptoutils.cpp
        e2ee/sssshandler.cpp
        keyimport.cpp
        thread.cpp
)

# Configure API files generation

set(CSAPI_DIR csapi)
set(ASAPI_DEF_DIR application-service/definitions)

set(API_GENERATION_ENABLED 0)
if (NOT MATRIX_SPEC_PATH AND MATRIX_DOC_PATH)
    set(MATRIX_SPEC_PATH ${MATRIX_DOC_PATH})
endif()
if (GTAD_PATH AND MATRIX_SPEC_PATH)
    # REALPATH resolves ~ (home directory) while PROGRAM doesn't
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" REALPATH)
    get_filename_component(ABS_GTAD_PATH "${ABS_GTAD_PATH}" PROGRAM PROGRAM_ARGS GTAD_ARGS)
    if (EXISTS ${ABS_GTAD_PATH})
        get_filename_component(ABS_API_DEF_PATH "${MATRIX_SPEC_PATH}/data/api" REALPATH)
        if (NOT IS_DIRECTORY ${ABS_API_DEF_PATH})
            # Check the old place of API files
            get_filename_component(ABS_API_DEF_PATH "${MATRIX_SPEC_PATH}/api" REALPATH)
        endif ()
        if (IS_DIRECTORY ${ABS_API_DEF_PATH})
            set(API_GENERATION_ENABLED 1)
        else ()
            message( WARNING "${MATRIX_SPEC_PATH} doesn't seem to point to a valid matrix-doc repo; disabling API stubs generation")
        endif ()
    else (EXISTS ${ABS_GTAD_PATH})
        message( WARNING "${GTAD_PATH} is not executable; disabling API stubs generation")
    endif ()
endif ()
if (API_GENERATION_ENABLED)
    message( STATUS "Using GTAD at ${ABS_GTAD_PATH}" )
    message( STATUS "Found API files at ${ABS_API_DEF_PATH}" )
    if (NOT CLANG_FORMAT)
        set(CLANG_FORMAT clang-format)
    endif()
    get_filename_component(ABS_CLANG_FORMAT "${CLANG_FORMAT}" PROGRAM PROGRAM_ARGS CLANG_FORMAT_ARGS)
    if (NOT ABS_CLANG_FORMAT)
        message( WARNING "${CLANG_FORMAT} is NOT FOUND; API files won't be formatted")
    endif ()

    set(FULL_CSAPI_SRC_DIR ${ABS_API_DEF_PATH}/client-server)
    file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
        ${FULL_CSAPI_SRC_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ASAPI_DEF_DIR}/*.yaml
    )
    add_custom_target(update-api
        ${ABS_GTAD_PATH} --config ../gtad/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            sync.yaml- # we have a better handcrafted implementation
            ${GTAD_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES gtad/gtad.yaml
                gtad/data.h.mustache
                gtad/operation.h.mustache
                gtad/operation.cpp.mustache
                gtad/preamble.mustache
                gtad/define_models.mustache
                ${API_DEFS}
        VERBATIM
    )
endif()
add_feature_info(EnableApiCodeGeneration "${API_GENERATION_ENABLED}"
                 "build target update-api")

# Produce the list of all Matrix API files for building the library. When this
# list changes (normally after calling GTAD), CONFIGURE_DEPENDS will force
# the build system to call CMake again. Checking for the glob change slows down
# each build (even if the target does not involve API generation). It would be
# ideal if GTAD could compare the initial (saved somewhere) and the generated
# file list itself and write down to some .cmake file if those are different,
# which would trigger the reconfiguration specifically before the next build.
# For now CONFIGURE_DEPENDS is the best approximation of that.

file(GLOB_RECURSE api_ALL_HEADERS CONFIGURE_DEPENDS ${CSAPI_DIR}/*.h ${ASAPI_DEF_DIR}/*.h)
target_sources(${QUOTIENT_LIB_NAME} PUBLIC FILE_SET HEADERS FILES ${api_ALL_HEADERS})

file(GLOB_RECURSE api_ALL_IMPLS CONFIGURE_DEPENDS ${CSAPI_DIR}/*.cpp)
target_sources(${QUOTIENT_LIB_NAME} PRIVATE ${api_ALL_IMPLS})


qt6_add_qml_module(QuotientQml
    URI quotient_im.Quotient
    VERSION ${API_VERSION}
    RESOURCE_PREFIX /qt/qml/quotient/imports
    OUTPUT_DIRECTORY quotient_im/Quotient
    IMPORT_PATH quotient_im/Quotient
)
