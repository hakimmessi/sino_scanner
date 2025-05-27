cmake_minimum_required(VERSION 3.10)
project(sino_scanner LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Flutter's CMake configuration
include(flutter/CMakeLists.txt)

# Define your custom sources
set(CUSTOM_SOURCES
        "runner/main.cc"
        "runner/my_application.cc"
        "src/sinosecu_wrapper.cpp"
)

# Define the executable using Flutter's function
flutter_add_executable(${FLUTTER_PROJECT_NAME}
        ${CUSTOM_SOURCES}
)

# Link GTK for UI
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE PkgConfig::GTK)

# *** Sinosecu SDK Linking ***
set(SINOSEC_SDK_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/nativeLibs")

# Check if the SDK directory exists
if(NOT EXISTS "${SINOSEC_SDK_LIB_DIR}")
    message(WARNING "Sinosecu SDK directory not found: ${SINOSEC_SDK_LIB_DIR}")
endif()

# List the SDK libraries
set(SINOSEC_LIBS
        "${SINOSEC_SDK_LIB_DIR}/libIDCard.so"
        # Add other libraries as needed:
        # "${SINOSEC_SDK_LIB_DIR}/libIDCardKernal.so"
        # "${SINOSEC_SDK_LIB_DIR}/libCamDll.so"
)

# Only link libraries that actually exist
foreach(LIB ${SINOSEC_LIBS})
    if(EXISTS "${LIB}")
        target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE "${LIB}")
    else()
        message(WARNING "Library not found: ${LIB}")
    endif()
endforeach()

# Set RPATH for runtime library discovery
set_target_properties(${FLUTTER_PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "\$ORIGIN/libs/nativeLibs"
        BUILD_WITH_INSTALL_RPATH TRUE
)

# Standard Flutter installation targets
install(TARGETS ${FLUTTER_PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
        LIBRARY DESTINATION lib
)

# Install Flutter assets
if(DEFINED FLUTTER_ICU_DATA_FILE)
    install(FILES "${FLUTTER_ICU_DATA_FILE}" DESTINATION "lib")
endif()

if(DEFINED FLUTTER_LIBRARY)
    install(FILES "${FLUTTER_LIBRARY}" DESTINATION "lib")
endif()

install(DIRECTORY "${FLUTTER_APP_DIR}/flutter_assets" DESTINATION "data")

# Install SDK libraries and dependencies
if(EXISTS "${SINOSEC_SDK_LIB_DIR}")
    install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
            DESTINATION "libs/nativeLibs"
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