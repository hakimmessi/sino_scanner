# Project-level configuration.
cmake_minimum_required(VERSION 3.13)
project(runner LANGUAGES CXX)

# The name of the executable created for the application. Change this to change
# the on-disk name of your application.
set(BINARY_NAME "sino_scanner")
# The unique GTK application identifier for this application. See:
# https://wiki.gnome.org/HowDoI/ChooseApplicationID
set(APPLICATION_ID "com.kinektek.sino_scanner")

# Explicitly opt in to modern CMake behaviors to avoid warnings with recent
# versions of CMake.
cmake_policy(SET CMP0063 NEW)

# Load bundled libraries from the lib/ directory relative to the binary.
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

# Root filesystem for cross-building.
if(FLUTTER_TARGET_PLATFORM_SYSROOT)
    set(CMAKE_SYSROOT ${FLUTTER_TARGET_PLATFORM_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# Define build configuration options.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE
            STRING "Flutter build mode" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Profile" "Release")
endif()

# Compilation settings that should be applied to most targets.
#
# Be cautious about adding new options here, as plugins use this function by
# default. In most cases, you should add new options to specific targets instead
# of modifying this function.
function(APPLY_STANDARD_SETTINGS TARGET)
    target_compile_features(${TARGET} PUBLIC cxx_std_14)
    target_compile_options(${TARGET} PRIVATE -Wall -Werror)
    target_compile_options(${TARGET} PRIVATE "$<$<NOT:$<CONFIG:Debug>>:-O3>")
    target_compile_definitions(${TARGET} PRIVATE "$<$<NOT:$<CONFIG:Debug>>:NDEBUG>")
endfunction()

# Flutter library and tool build rules.
set(FLUTTER_MANAGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/flutter")
add_subdirectory(${FLUTTER_MANAGED_DIR})

# System-level dependencies.
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

# Application build; see runner/CMakeLists.txt.
add_subdirectory("runner")

# Run the Flutter tool portions of the build. This must not be removed.
add_dependencies(${BINARY_NAME} flutter_assemble)

# Only the install-generated bundle's copy of the executable will launch
# correctly, since the resources must in the right relative locations. To avoid
# people trying to run the unbundled copy, put it in a subdirectory instead of
# the default top-level location.
set_target_properties(${BINARY_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/intermediates_do_not_run"
)

# Generated plugin build rules, which manage building the plugins and adding
# them to the application.
include(flutter/generated_plugins.cmake)

# === Installation ===
# By default, "installing" just makes a relocatable bundle in the build
# directory.
set(BUILD_BUNDLE_DIR "${PROJECT_BINARY_DIR}/bundle")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${BUILD_BUNDLE_DIR}" CACHE PATH "..." FORCE)
endif()

# Start with a clean build bundle directory every time.
install(CODE "
  file(REMOVE_RECURSE \"${BUILD_BUNDLE_DIR}/\")
  " COMPONENT Runtime)

set(INSTALL_BUNDLE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/data")
set(INSTALL_BUNDLE_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")

install(TARGETS ${BINARY_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}"
        COMPONENT Runtime)

install(FILES "${FLUTTER_ICU_DATA_FILE}" DESTINATION "${INSTALL_BUNDLE_DATA_DIR}"
        COMPONENT Runtime)

install(FILES "${FLUTTER_LIBRARY}" DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
        COMPONENT Runtime)

foreach(bundled_library ${PLUGIN_BUNDLED_LIBRARIES})
    install(FILES "${bundled_library}"
            DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
            COMPONENT Runtime)
endforeach(bundled_library)

# Copy the native assets provided by the build.dart from all packages.
set(NATIVE_ASSETS_DIR "${PROJECT_BUILD_DIR}native_assets/linux/")
install(DIRECTORY "${NATIVE_ASSETS_DIR}"
        DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
        COMPONENT Runtime)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    add_compile_definitions(ARM_BUILD)
    # Add any ARM-specific compiler flags here
endif()

# Fully re-copy the assets directory on each build to avoid having stale files
# from a previous install.
set(FLUTTER_ASSET_DIR_NAME "flutter_assets")
install(CODE "
  file(REMOVE_RECURSE \"${INSTALL_BUNDLE_DATA_DIR}/${FLUTTER_ASSET_DIR_NAME}\")
  " COMPONENT Runtime)
install(DIRECTORY "${PROJECT_BUILD_DIR}/${FLUTTER_ASSET_DIR_NAME}"
        DESTINATION "${INSTALL_BUNDLE_DATA_DIR}" COMPONENT Runtime)

# Install the AOT library on non-Debug builds only.
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    install(FILES "${AOT_LIBRARY}" DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
            COMPONENT Runtime)
endif()

# *** Custom sources for your application ***
target_sources(${BINARY_NAME}
        PRIVATE
        runner/main.cc
        runner/my_application.cc
        src/sinosecu_wrapper.cpp
)

# *** Sinosecu SDK Linking ***
set(SINOSEC_SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/nativeLibs")

# Check if the SDK directory exists
if(NOT EXISTS "${SINOSEC_SDK_LIB_DIR}")
    message(WARNING "Sinosecu SDK directory not found: ${SINOSEC_SDK_LIB_DIR}")
endif()

# List the SDK libraries
set(SINOSEC_LIBS
        "${SINOSEC_SDK_LIB_DIR}/libIDCard.so"
        # Add other libraries as needed:
        "${SINOSEC_SDK_LIB_DIR}/libIDCardKernal.so"
         "${SINOSEC_SDK_LIB_DIR}/libCamDll.so"
)

# Link SDK libraries to the binary
foreach(LIB ${SINOSEC_LIBS})
    if(EXISTS "${LIB}")
        target_link_libraries(${BINARY_NAME} PRIVATE
                flutter
                "${LIB}")
    else()
        message(WARNING "Library not found: ${LIB}")
    endif()
endforeach()

# Set RPATH for SDK libraries - simplified to just look in lib directory
set_target_properties(${BINARY_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
)

# Install SDK libraries directly to the main lib directory (not nested)
if(EXISTS "${SINOSEC_SDK_LIB_DIR}")
    install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
            DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
            FILES_MATCHING
            PATTERN "*.so*"
            PATTERN "*.dat"
            PATTERN "*.xml"
            PATTERN "*.ini"
            PATTERN "*.txt"
            PATTERN "*.cpp" EXCLUDE
            PATTERN "*.h" EXCLUDE
    )
endif()

# *** Fix Architecture Mismatch Issues ***
if(IS_ARM64)
    message(STATUS "Applying ARM64 fixes for SDK libraries...")

    # Libraries that need to be excluded or replaced on ARM64
    set(X86_ONLY_LIBS
            "libpng15.so.15.13.0"  # This is x86-64, use system version
            # Add other x86-64 libraries here if found
    )

    # Use system libraries for architecture-specific dependencies
    find_package(PNG REQUIRED)
    if(PNG_FOUND)
        message(STATUS "Using system PNG library instead of bundled x86-64 version")
        target_link_libraries(${BINARY_NAME} PRIVATE PNG::PNG)
    endif()

    # Install SDK libraries but exclude x86-64 ones
    install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
            DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
            FILES_MATCHING
            PATTERN "*.so*"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )

    # Remove x86-64 libraries after installation
    foreach(X86_LIB ${X86_ONLY_LIBS})
        install(CODE "
            if(EXISTS \"${INSTALL_BUNDLE_LIB_DIR}/${X86_LIB}\")
                file(REMOVE \"${INSTALL_BUNDLE_LIB_DIR}/${X86_LIB}\")
                message(STATUS \"Removed x86-64 library: ${X86_LIB}\")
            endif()
        ")
    endforeach()

else()
    # For x86-64 builds, install all libraries normally
    install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
            DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
            FILES_MATCHING
            PATTERN "*.so*"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()