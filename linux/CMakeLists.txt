cmake_minimum_required(VERSION 3.10)
project(sino_scanner LANGUAGES CXX C) # Ensure CXX is listed for C++


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(flutter/CMakeLists.txt)

set(CUSTOM_SOURCES
        "runner/main.cc"
        "runner/my_application.cc"
        "src/sinosecu_wrapper.cpp"
)

# Define the executable
flutter_add_executable(${FLUTTER_PROJECT_NAME}
        ${CUSTOM_SOURCES})

# Link GTK for UI
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE PkgConfig::GTK)

# *** Sinosecu SDK Linking ***
# Specify the directory where libIDCard.so and other SDK .so files are
set(SINOSEC_SDK_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/nativeLibs")

# List the specific .so files from the SDK you need to link against
# At a minimum, libIDCard.so. You might need others based on SDK docs.
#  you also have libIDCardKernal.so, libCamDll.so etc.
# Start with libIDCard.so and add others if you get linking/runtime errors.
set(SINOSEC_LIBS
        "${SINOSEC_SDK_LIB_DIR}/libIDCard.so"
        # "${SINOSEC_SDK_LIB_DIR}/libIDCardKernal.so"
        # "${SINOSEC_SDK_LIB_DIR}/libusb-1.0.so" # If SDK depends on it and it's not system-wide
)

target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE ${SINOSEC_LIBS})

# Set RPATH so the executable can find the SDK .so files at runtime
# This tells the executable to look in a 'libs/nativeLibs' folder relative to itself.
target_link_options(${FLUTTER_PROJECT_NAME} PRIVATE "-Wl,-rpath,'\$\$ORIGIN/libs/nativeLibs'")
# **************************


install(TARGETS ${FLUTTER_PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
        LIBRARY DESTINATION lib)

install(FILES "${FLUTTER_APP_DIR}/icudtl.dat" DESTINATION "lib")

install(DIRECTORY "${FLUTTER_APP_DIR}/flutter_assets" DESTINATION "data")

# Install your SDK's .so files and other necessary runtime files alongside your app
# This ensures they are copied to the build output directory in the correct relative path for RPATH.
install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
        DESTINATION "lib/libs/nativeLibs" # This will create build/linux/x64/release/bundle/libs/nativeLibs
        FILES_MATCHING PATTERN "*.so*" PATTERN "*.dat" PATTERN "*.xml" PATTERN "*.ini" PATTERN "*.txt" # Add patterns for all required SDK files
        # Exclude source files if any are in there by mistake
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.h" EXCLUDE
)