cmake_minimum_required(VERSION 3.31)

if(APPLE)
    project(ShatterDelay VERSION 0.1.0 LANGUAGES C CXX OBJC OBJCXX)
else()
    project(ShatterDelay VERSION 0.1.0 LANGUAGES C CXX)
endif()

option(SHATTERDELAY_BUILD_PLUGIN "Build the YUP Standalone/VST3/AU targets" ON)
option(SHATTERDELAY_BUILD_TESTS "Build deterministic DSP regression tests" ON)
option(SHATTERDELAY_ADHOC_SIGN "Ad-hoc sign local macOS plugin and app bundles after building" ON)
set(SHATTERDELAY_CREATE_AU OFF)
if(APPLE)
    set(SHATTERDELAY_CREATE_AU ON)
endif()
set(SHATTERDELAY_YUP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../yup" CACHE PATH
    "Path to a local YUP checkout; fetched from upstream when unavailable")

add_library(shatterdelay_engine STATIC
    source/ShatterDelayEngine.cpp)
target_include_directories(shatterdelay_engine PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_features(shatterdelay_engine PUBLIC cxx_std_20)
set_target_properties(shatterdelay_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(shatterdelay_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SHATTERDELAY_BUILD_TESTS)
    include(CTest)

    function(enable_test_assertions target_name)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /UNDEBUG)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE -UNDEBUG)
        endif()
    endfunction()

    add_executable(shatterdelay_engine_tests
        tests/ShatterDelayEngineTests.cpp)
    target_link_libraries(shatterdelay_engine_tests PRIVATE shatterdelay_engine)
    target_compile_features(shatterdelay_engine_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(shatterdelay_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    enable_test_assertions(shatterdelay_engine_tests)
    add_test(NAME shatterdelay_engine_tests COMMAND shatterdelay_engine_tests)
endif()

if(SHATTERDELAY_BUILD_PLUGIN)
    set(YUP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(YUP_BUILD_PLUGINS OFF CACHE BOOL "" FORCE)
    set(YUP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_PLUGINVAL OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_VST3_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_CLAP_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_AUVAL_VALIDATOR OFF CACHE BOOL "" FORCE)

    if(EXISTS "${SHATTERDELAY_YUP_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${SHATTERDELAY_YUP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/yup")
    else()
        include(FetchContent)
        FetchContent_Declare(yup
            GIT_REPOSITORY https://github.com/kunitoki/yup.git
            GIT_TAG 9a1c9bc699b6a714f6f52486462d98a140c8bf95)
        FetchContent_MakeAvailable(yup)
    endif()

    # YUP's iconutil-based helper rejects its generated iconset on current
    # macOS. Keep the compatibility fix project-local until upstream changes.
    if(APPLE)
        include(cmake/YupMacOSIconWorkaround.cmake)
    endif()

    yup_audio_plugin(
        TARGET_NAME shatterdelay
        TARGET_VERSION ${PROJECT_VERSION}
        TARGET_IDE_GROUP "ShatterDelay"
        TARGET_APP_ID "jp.ehl.shatterdelay"
        TARGET_APP_NAMESPACE "jp.ehl"
        TARGET_CXX_STANDARD 20
        PLUGIN_ID "jp.ehl.shatterdelay"
        PLUGIN_NAME "ShatterDelay"
        PLUGIN_VENDOR "ehl_"
        PLUGIN_EMAIL ""
        PLUGIN_VERSION "${PROJECT_VERSION}"
        PLUGIN_DESCRIPTION "Fractional stereo delay with filtered feedback and deterministic fracture bursts"
        PLUGIN_URL "https://github.com/EsionHsrahLatigid/ShatterDelay"
        PLUGIN_IS_SYNTH OFF
        PLUGIN_IS_MONO OFF
        PLUGIN_CREATE_CLAP OFF
        PLUGIN_CREATE_VST3 ON
        PLUGIN_CREATE_AU ${SHATTERDELAY_CREATE_AU}
        PLUGIN_AU_SUBTYPE "ShDl"
        PLUGIN_AU_MANUFACTURER "EHL1"
        PLUGIN_AU_SANDBOX_SAFE ON
        PLUGIN_CREATE_AUv3 OFF
        PLUGIN_CREATE_LV2 OFF
        PLUGIN_CREATE_AAX OFF
        PLUGIN_CREATE_STANDALONE ON
        PLUGIN_COPY_AFTER_BUILD OFF
        MODULES
            yup::yup_gui
            yup::yup_audio_processors)

    target_sources(shatterdelay_shared INTERFACE
        source/ParameterGridEditor.cpp
        source/ParameterGridEditor.h
        source/ProductState.h
        source/ShatterDelayPlugin.cpp
        source/ShatterDelayPlugin.h)
    target_link_libraries(shatterdelay_shared INTERFACE shatterdelay_engine)
    target_include_directories(shatterdelay_shared INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/source")

    if(SHATTERDELAY_BUILD_TESTS)
        add_executable(shatterdelay_plugin_bridge_tests
            tests/ShatterDelayPluginBridgeTests.cpp
            source/ShatterDelayPlugin.cpp)
        target_link_libraries(shatterdelay_plugin_bridge_tests PRIVATE shatterdelay_engine yup::yup_audio_processors)
        target_include_directories(shatterdelay_plugin_bridge_tests PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include"
            "${CMAKE_CURRENT_LIST_DIR}/source")
        target_compile_definitions(shatterdelay_plugin_bridge_tests PRIVATE SHATTERDELAY_HEADLESS_TEST=1)
        target_compile_features(shatterdelay_plugin_bridge_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(shatterdelay_plugin_bridge_tests PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        enable_test_assertions(shatterdelay_plugin_bridge_tests)
        add_test(NAME shatterdelay_plugin_bridge_tests COMMAND shatterdelay_plugin_bridge_tests)

        add_executable(shatterdelay_standalone_bridge_tests
            tests/ShatterDelayStandaloneBridgeTests.cpp
            source/ShatterDelayPlugin.cpp)
        target_link_libraries(shatterdelay_standalone_bridge_tests PRIVATE shatterdelay_engine yup::yup_audio_processors)
        target_include_directories(shatterdelay_standalone_bridge_tests PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include"
            "${CMAKE_CURRENT_LIST_DIR}/source")
        target_compile_definitions(shatterdelay_standalone_bridge_tests PRIVATE
            SHATTERDELAY_HEADLESS_TEST=1
            YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1)
        target_compile_features(shatterdelay_standalone_bridge_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(shatterdelay_standalone_bridge_tests PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        enable_test_assertions(shatterdelay_standalone_bridge_tests)
        add_test(NAME shatterdelay_standalone_bridge_tests COMMAND shatterdelay_standalone_bridge_tests)
    endif()

    set(shatterdelay_release_bundle_targets
        shatterdelay_standalone_plugin
        shatterdelay_vst3_plugin)
    if(APPLE)
        list(APPEND shatterdelay_release_bundle_targets shatterdelay_au_plugin)
    endif()

    add_custom_target(shatterdelay_release_bundles
        DEPENDS ${shatterdelay_release_bundle_targets})

    include(cmake/EhlYupArtifactLayout.cmake)
    ehl_yup_add_artifact_target(
        TARGET ehl_stage_products
        PRODUCT ShatterDelay
        SLUG shatterdelay
        STANDALONE_TARGET shatterdelay_standalone_plugin
        VST3_TARGET shatterdelay_vst3_plugin
        AU_TARGET shatterdelay_au_plugin)

    if(APPLE)
        set_target_properties(shatterdelay_standalone_plugin PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "jp.ehl.shatterdelay"
            MACOSX_BUNDLE_BUNDLE_NAME "ShatterDelay"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "jp.ehl.shatterdelay")
    endif()

    if(APPLE AND SHATTERDELAY_ADHOC_SIGN)
        find_program(SHATTERDELAY_CODESIGN_EXECUTABLE codesign REQUIRED)
        add_custom_command(TARGET shatterdelay_vst3_plugin POST_BUILD
            COMMAND "${SHATTERDELAY_CODESIGN_EXECUTABLE}" --force --deep --sign -
                    "$<TARGET_BUNDLE_DIR:shatterdelay_vst3_plugin>"
            COMMENT "Ad-hoc signing ShatterDelay VST3 for local loading"
            VERBATIM)
        add_custom_command(TARGET shatterdelay_standalone_plugin POST_BUILD
            COMMAND "${SHATTERDELAY_CODESIGN_EXECUTABLE}" --force --deep --sign -
                    "$<TARGET_BUNDLE_DIR:shatterdelay_standalone_plugin>"
            COMMENT "Ad-hoc signing ShatterDelay standalone app for local launch"
            VERBATIM)
    endif()
endif()
