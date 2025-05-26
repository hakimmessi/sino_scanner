cmake_minimum_required(VERSION 3.10)
project(sino_scanner LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLUTTER_PROJECT_NAME "sino_scanner")
set(FLUTTER_APPLICATION_ID "com.example.sino_scanner")

# Add necessary include directories relative to this CMakeLists.txt (linux/)
include_directories(
        "runner"
        "src"
        "flutter/ephemeral"
)

# Define sources for the application executable
set(SOURCES
        "runner/main.cc"
        "runner/my_application.cc"
        "src/sinosecu_wrapper.cpp"
        "flutter/generated_plugin_registrant.cc"
)

# Define the executable
add_executable(${FLUTTER_PROJECT_NAME} "${SOURCES}")

# Link Flutter library
target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE flutter)

# Link GTK for UI
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE PkgConfig::GTK)

# *** Sinosecu SDK Linking ***
set(SINOSEC_SDK_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/nativeLibs")
set(SINOSEC_LIBS
        "${SINOSEC_SDK_LIB_DIR}/libIDCard.so"
)
target_link_libraries(${FLUTTER_PROJECT_NAME} PRIVATE ${SINOSEC_LIBS})
target_link_options(${FLUTTER_PROJECT_NAME} PRIVATE "-Wl,-rpath,'\$\$ORIGIN/libs/nativeLibs'")

# ... (rest of your install commands) ...
install(TARGETS ${FLUTTER_PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
        LIBRARY DESTINATION lib)

install(FILES "${CMAKE_BINARY_DIR}/flutter/icudtl.dat" DESTINATION "lib")

install(DIRECTORY "${CMAKE_BINARY_DIR}/flutter_assets" DESTINATION "data")

install(DIRECTORY "${SINOSEC_SDK_LIB_DIR}/"
        DESTINATION "lib/libs/nativeLibs"
        FILES_MATCHING PATTERN "*.so*" PATTERN "*.dat" PATTERN "*.xml" PATTERN "*.ini" PATTERN "*.txt"
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.h" EXCLUDE
)