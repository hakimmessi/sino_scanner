//
// Created by Hakim Messi on 2025/06/03.
//

#ifndef SINO_SCANNER_PNG_WRAPPER_H
#define SINO_SCANNER_PNG_WRAPPER_H

#include <png.h>
#include <memory>
#include <cstdio>

// Forward declare CDib class - you'll need to match this to the actual structure
struct CDib;

/**
 * PNG Wrapper Class
 *
 * This class provides a safe wrapper around PNG operations
 * to fix issues with statically linked PNG code in libIDCard.so
 * that was compiled with PNG15 but has ARM64 compatibility issues.
 *
 * Strategy: Override the high-level read_png_file functions instead
 * of trying to replace 300+ individual PNG functions.
 */
class PngWrapper {
public:
    PngWrapper();
    ~PngWrapper();

    // Initialize the wrapper and load system PNG library
    bool initialize();

    // Clean up resources
    void cleanup();

    // High-level PNG reading functions
    int readPngFromFile(CDib* dib, FILE* fp);
    int readPngFromPath(CDib* dib, const char* filename);

    // Check if wrapper is properly initialized
    bool isInitialized() const { return systemPngHandle != nullptr; }

    // Get singleton instance
    static PngWrapper& getInstance();

    // Helper to get function pointer from system PNG library
    template<typename T>
    T getSystemFunction(const char* functionName);

private:
    void* systemPngHandle;
    bool initialized;

    // PNG processing helpers
    bool loadPngIntoBuffer(FILE* fp, unsigned char** imageData,
                           int* width, int* height, int* channels);
    bool convertToCDib(CDib* dib, unsigned char* imageData,
                       int width, int height, int channels);

    // Prevent copying
    PngWrapper(const PngWrapper&) = delete;
    PngWrapper& operator=(const PngWrapper&) = delete;
};

// C-style wrapper functions that will intercept PNG calls
extern "C" {

// ================================
// HIGH-LEVEL FUNCTION OVERRIDES (PRIMARY TARGET)
// ================================

// These are the main functions libIDCard.so calls - intercept these first
int read_png_file(CDib* dib, FILE* fp);
int read_png_file2(CDib* dib, char* filename);

// ================================
// LOW-LEVEL FUNCTION OVERRIDES (FALLBACK)
// ================================

png_structp png_create_read_struct(png_const_charp user_png_ver,
                                   png_voidp error_ptr,
                                   png_error_ptr error_fn,
                                   png_error_ptr warn_fn);

png_infop png_create_info_struct(png_const_structrp png_ptr);

void png_destroy_read_struct(png_structpp png_ptr_ptr,
                             png_infopp info_ptr_ptr,
                             png_infopp end_info_ptr_ptr);

// Add other critical PNG functions as needed
void png_read_info(png_structrp png_ptr, png_inforp info_ptr);
void png_read_image(png_structrp png_ptr, png_bytepp image);
void png_read_end(png_structrp png_ptr, png_inforp info_ptr);
}

#endif //SINO_SCANNER_PNG_WRAPPER_H