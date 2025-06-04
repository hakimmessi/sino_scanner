//
// Created by Hakim Messi on 2025/06/03.
//
#include "png_wrapper.h"
#include <dlfcn.h>
#include <iostream>
#include <cstdlib>
#include <png.h>
#include <cstring>
#include <cstdio>

// Forward declare CDib class (need to reverse engineer this)
// This is a placeholder - you'll need to match the actual structure
struct CDib {
    int width;
    int height;
    int bitsPerPixel;
    unsigned char* imageData;
    size_t dataSize;
    // Add more fields as you discover them
};

// ================================
// DEBUG FUNCTIONS
// ================================

void debugCDib(CDib* dib, const char* context = "") {
    if (!dib) {
        std::cout << "CDib debug [" << context << "]: NULL pointer" << std::endl;
        return;
    }

    std::cout << "=== CDib debug [" << context << "] ===" << std::endl;
    std::cout << "CDib pointer: " << (void*)dib << std::endl;
    std::cout << "CDib size: " << sizeof(CDib) << " bytes" << std::endl;

    // Print the CDib structure as it currently exists
    std::cout << "Current CDib values:" << std::endl;
    std::cout << "  width: " << dib->width << std::endl;
    std::cout << "  height: " << dib->height << std::endl;
    std::cout << "  bitsPerPixel: " << dib->bitsPerPixel << std::endl;
    std::cout << "  imageData: " << (void*)dib->imageData << std::endl;
    std::cout << "  dataSize: " << dib->dataSize << std::endl;

    // Print first 128 bytes of the CDib structure as hex
    std::cout << "Raw CDib memory (first 128 bytes):" << std::endl;
    unsigned char* ptr = (unsigned char*)dib;
    for (int i = 0; i < 128 && i < sizeof(CDib) * 4; i++) {
        printf("%02x ", ptr[i]);
        if (i % 16 == 15) printf("\n");
    }
    if (128 % 16 != 0) printf("\n");

    std::cout << "=========================" << std::endl;
}

void analyzeCDibUsage(CDib* dib, const char* operation) {
    static int call_count = 0;
    call_count++;

    std::cout << "\n*** CDib Analysis Call #" << call_count << " [" << operation << "] ***" << std::endl;

    if (dib) {
        // Check if CDib looks pre-initialized
        bool looks_initialized = (dib->width > 0 && dib->width < 10000) &&
                                 (dib->height > 0 && dib->height < 10000) &&
                                 (dib->bitsPerPixel > 0 && dib->bitsPerPixel <= 32);

        std::cout << "CDib appears " << (looks_initialized ? "PRE-INITIALIZED" : "UNINITIALIZED") << std::endl;

        if (looks_initialized) {
            std::cout << "Expected image size: " << dib->width << "x" << dib->height
                      << " (" << dib->bitsPerPixel << "bpp)" << std::endl;
            std::cout << "Expected data size: " << (dib->width * dib->height * dib->bitsPerPixel / 8) << " bytes" << std::endl;
        }

        debugCDib(dib, operation);
    } else {
        std::cout << "CDib is NULL!" << std::endl;
    }

    std::cout << "*** End Analysis Call #" << call_count << " ***\n" << std::endl;
}

// Static instance for singleton pattern
static PngWrapper* g_instance = nullptr;

PngWrapper::PngWrapper() : systemPngHandle(nullptr), initialized(false) {}

PngWrapper::~PngWrapper() {
    cleanup();
}

bool PngWrapper::initialize() {
    if (initialized) {
        return true;
    }

    std::cout << "PngWrapper: Initializing system PNG library..." << std::endl;

    // Try to load system PNG library
    const char* png_libs[] = {
            "libpng16.so.16",
            "libpng16.so",
            "libpng.so",
            "/usr/lib/aarch64-linux-gnu/libpng16.so.16"
    };

    for (const char* lib : png_libs) {
        systemPngHandle = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
        if (systemPngHandle) {
            std::cout << "PngWrapper: Successfully loaded " << lib << std::endl;
            break;
        }
    }

    if (!systemPngHandle) {
        std::cerr << "PngWrapper: Failed to load system PNG library: " << dlerror() << std::endl;
        return false;
    }

    // Verify we can find key functions
    if (!getSystemFunction<void*>("png_create_read_struct")) {
        std::cerr << "PngWrapper: System PNG library missing required functions" << std::endl;
        cleanup();
        return false;
    }

    initialized = true;
    std::cout << "PngWrapper: Initialization successful!" << std::endl;
    return true;
}

void PngWrapper::cleanup() {
    if (systemPngHandle) {
        dlclose(systemPngHandle);
        systemPngHandle = nullptr;
    }
    initialized = false;
}

template<typename T>
T PngWrapper::getSystemFunction(const char* functionName) {
    if (!systemPngHandle) {
        return nullptr;
    }

    void* func = dlsym(systemPngHandle, functionName);
    if (!func) {
        std::cerr << "PngWrapper: Function not found: " << functionName
                  << " Error: " << dlerror() << std::endl;
    }
    return reinterpret_cast<T>(func);
}

// ================================
// HIGH-LEVEL PNG READING FUNCTIONS
// ================================

bool PngWrapper::loadPngIntoBuffer(FILE* fp, unsigned char** imageData,
                                   int* width, int* height, int* channels) {
    if (!initialized && !initialize()) {
        return false;
    }

    // Use system PNG16 library functions via dlsym
    typedef png_structp (*create_read_func)(png_const_charp, png_voidp, png_error_ptr, png_error_ptr);
    typedef png_infop (*create_info_func)(png_const_structrp);
    typedef void (*destroy_read_func)(png_structpp, png_infopp, png_infopp);
    typedef void (*init_io_func)(png_structrp, FILE*);
    typedef void (*read_info_func)(png_structrp, png_inforp);
    typedef void (*read_image_func)(png_structrp, png_bytepp);
    typedef void (*read_end_func)(png_structrp, png_inforp);
    typedef void (*read_update_info_func)(png_structrp, png_inforp);

    auto png_create_read_struct = getSystemFunction<create_read_func>("png_create_read_struct");
    auto png_create_info_struct = getSystemFunction<create_info_func>("png_create_info_struct");
    auto png_destroy_read_struct = getSystemFunction<destroy_read_func>("png_destroy_read_struct");
    auto png_init_io = getSystemFunction<init_io_func>("png_init_io");
    auto png_read_info = getSystemFunction<read_info_func>("png_read_info");
    auto png_read_image = getSystemFunction<read_image_func>("png_read_image");
    auto png_read_end = getSystemFunction<read_end_func>("png_read_end");
    auto png_read_update_info = getSystemFunction<read_update_info_func>("png_read_update_info");

    if (!png_create_read_struct || !png_create_info_struct || !png_init_io || !png_read_info) {
        std::cerr << "Failed to load required PNG functions" << std::endl;
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        std::cerr << "Failed to create PNG read struct" << std::endl;
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        std::cerr << "Failed to create PNG info struct" << std::endl;
        return false;
    }

    // Set up error handling (simplified)
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::cerr << "PNG reading error occurred" << std::endl;
        return false;
    }

    // Read PNG file
    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    // Get image dimensions and properties
    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    // Convert to standard format (8-bit RGBA)
    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    *channels = png_get_channels(png_ptr, info_ptr);
    size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    // Allocate memory for image
    *imageData = (unsigned char*)malloc(row_bytes * (*height));
    if (!*imageData) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    // Create row pointers
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++) {
        row_pointers[y] = (*imageData) + (y * row_bytes);
    }

    // Read the image
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, nullptr);

    // Cleanup
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    std::cout << "Successfully loaded PNG: " << *width << "x" << *height
              << " channels=" << *channels << std::endl;
    return true;
}

bool PngWrapper::convertToCDib(CDib* dib, unsigned char* imageData,
                               int width, int height, int channels) {
    if (!dib) return false;

    std::cout << "Converting to CDib format..." << std::endl;

    // IMPORTANT: Before modifying CDib, let's see what was there originally
    debugCDib(dib, "convertToCDib - BEFORE modification");

    // Set CDib properties
    dib->width = width;
    dib->height = height;
    dib->bitsPerPixel = channels * 8;
    dib->dataSize = width * height * channels;

    // Check if CDib already has allocated memory
    if (dib->imageData != nullptr) {
        std::cout << "WARNING: CDib already has imageData allocated at "
                  << (void*)dib->imageData << std::endl;
        std::cout << "This might mean CDib manages its own memory!" << std::endl;
        // Don't free it yet - let's see what happens
    }

    // Allocate memory for CDib (this might not be the right approach)
    dib->imageData = (unsigned char*)malloc(dib->dataSize);
    if (!dib->imageData) {
        std::cerr << "Failed to allocate memory for CDib" << std::endl;
        return false;
    }

    // Copy image data
    memcpy(dib->imageData, imageData, dib->dataSize);

    std::cout << "CDib conversion completed: " << width << "x" << height
              << " " << dib->bitsPerPixel << "bpp" << std::endl;

    // Show final state
    debugCDib(dib, "convertToCDib - AFTER modification");

    return true;
}

int PngWrapper::readPngFromFile(CDib* dib, FILE* fp) {
    unsigned char* imageData = nullptr;
    int width, height, channels;

    if (!loadPngIntoBuffer(fp, &imageData, &width, &height, &channels)) {
        std::cerr << "Failed to load PNG from file pointer" << std::endl;
        return -1;
    }

    bool success = convertToCDib(dib, imageData, width, height, channels);
    free(imageData);

    return success ? 0 : -1;
}

int PngWrapper::readPngFromPath(CDib* dib, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Cannot open PNG file: " << filename << std::endl;
        return -1;
    }

    int result = readPngFromFile(dib, fp);
    fclose(fp);

    return result;
}

PngWrapper& PngWrapper::getInstance() {
    if (!g_instance) {
        g_instance = new PngWrapper();
        std::atexit([]() {
            delete g_instance;
            g_instance = nullptr;
        });
    }
    return *g_instance;
}

// ================================
// HIGH-LEVEL FUNCTION OVERRIDES
// ================================

extern "C" {

// Override the main PNG reading functions from libIDCard.so
int read_png_file(CDib* dib, FILE* fp) {
    std::cout << "*** INTERCEPTED: read_png_file(CDib*, FILE*) ***" << std::endl;

    // CRITICAL: Analyze the CDib structure BEFORE we modify it
    analyzeCDibUsage(dib, "read_png_file - BEFORE processing");

    if (!dib || !fp) {
        std::cerr << "Invalid parameters to read_png_file" << std::endl;
        return -1;
    }

    int result = PngWrapper::getInstance().readPngFromFile(dib, fp);

    // Analyze CDib AFTER our processing
    analyzeCDibUsage(dib, "read_png_file - AFTER processing");

    return result;
}

int read_png_file2(CDib* dib, char* filename) {
    std::cout << "*** INTERCEPTED: read_png_file2(CDib*, char*) ***" << std::endl;

    // CRITICAL: Analyze the CDib structure BEFORE we modify it
    analyzeCDibUsage(dib, "read_png_file2 - BEFORE processing");

    if (!dib || !filename) {
        std::cerr << "Invalid parameters to read_png_file2" << std::endl;
        return -1;
    }

    std::cout << "Attempting to read PNG file: " << filename << std::endl;

    int result = PngWrapper::getInstance().readPngFromPath(dib, filename);

    // Analyze CDib AFTER our processing
    analyzeCDibUsage(dib, "read_png_file2 - AFTER processing");

    return result;
}

// Keep the low-level PNG function overrides as fallback
/*
png_structp png_create_read_struct(png_const_charp user_png_ver,
                                   png_voidp error_ptr,
                                   png_error_ptr error_fn,
                                   png_error_ptr warn_fn) {
    std::cout << "*** FALLBACK INTERCEPTED: png_create_read_struct ***" << std::endl;

    auto& wrapper = PngWrapper::getInstance();
    if (!wrapper.isInitialized() && !wrapper.initialize()) {
        return nullptr;
    }

    typedef png_structp (*FuncType)(png_const_charp, png_voidp, png_error_ptr, png_error_ptr);
    auto func = wrapper.getSystemFunction<FuncType>("png_create_read_struct");

    if (func) {
        std::cout << "Creating read struct via system PNG" << std::endl;
        return func(user_png_ver, error_ptr, error_fn, warn_fn);
    }

    return nullptr;
}

png_infop png_create_info_struct(png_const_structrp png_ptr) {
    std::cout << "*** FALLBACK INTERCEPTED: png_create_info_struct ***" << std::endl;

    auto& wrapper = PngWrapper::getInstance();
    typedef png_infop (*FuncType)(png_const_structrp);
    auto func = wrapper.getSystemFunction<FuncType>("png_create_info_struct");

    if (func) {
        return func(png_ptr);
    }

    return nullptr;
}

void png_destroy_read_struct(png_structpp png_ptr_ptr,
                             png_infopp info_ptr_ptr,
                             png_infopp end_info_ptr_ptr) {
    std::cout << "*** FALLBACK INTERCEPTED: png_destroy_read_struct ***" << std::endl;

    auto& wrapper = PngWrapper::getInstance();
    typedef void (*FuncType)(png_structpp, png_infopp, png_infopp);
    auto func = wrapper.getSystemFunction<FuncType>("png_destroy_read_struct");

    if (func && png_ptr_ptr && *png_ptr_ptr) {
        func(png_ptr_ptr, info_ptr_ptr, end_info_ptr_ptr);
    }
} */

} // extern "C"