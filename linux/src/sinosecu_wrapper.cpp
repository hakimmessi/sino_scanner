#include "sinosecu_wrapper.h"
#include <iostream>
#include <codecvt>
#include <locale>

// --- String Conversion Utility ---
// Convert a std::string (UTF-8) to std::wstring (platform-dependent wchar_t, often UTF-32 on Linux)
std::wstring string_to_wstring(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter; // UTF-16 intermediate
        return converter.from_bytes(str);
    } catch (const std::range_error& e) {
        std::cerr << "WString conversion failed (range_error): " << e.what() << std::endl;
        // Return an empty wstring or handle error appropriately
        // This might happen if the input string is not valid UTF-8
        return L"";
    }
}


// --- SinosecuScanner Class Implementation ---

SinosecuScanner::SinosecuScanner() {
    // Constructor: Called when a SinosecuScanner object is created.
    // You could do initial setup here if needed, but for now, it's empty.
    std::cout << "SinosecuScanner object created." << std::endl;
}

SinosecuScanner::~SinosecuScanner() {
    // Destructor: Called when the SinosecuScanner object is destroyed.
    // Good place to ensure resources are released, though we'll do it explicitly with releaseScanner().
    std::cout << "SinosecuScanner object destroyed." << std::endl;
}

int SinosecuScanner::initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory) {
    // Convert std::string to const wchar_t* for the SDK
    std::wstring wUserId = string_to_wstring(userId);
    std::wstring wSdkDirectory = string_to_wstring(sdkDirectory);

    // LPCWSTR is 'Long Pointer to Constant Wide String', equivalent to const wchar_t*
    const wchar_t* lpUserId = wUserId.c_str();
    const wchar_t* lpDirectory = wSdkDirectory.c_str();

    std::wcout << L"Attempting to initialize Sinosecu SDK with:" << std::endl;
    std::wcout << L"  UserID: " << lpUserId << std::endl;
    std::wcout << L"  nType: " << nType << std::endl;
    std::wcout << L"  Directory: " << lpDirectory << std::endl;

    // Call the SDK's InitIDCard function
    int result = InitIDCard(lpUserId, nType, lpDirectory); // [cite: 29]

    // Check the result
    if (result == 0) {
        std::cout << "Sinosecu SDK initialized successfully." << std::endl;
    } else {
        std::cerr << "Sinosecu SDK initialization failed. Error code: " << result << std::endl;
        // this example below are from the SDK docs,
        // 1: Authorization ID is incorrect
        // 2: Device initialization is failed
        // 3: Recognition engine initialization is failed
        // 4: Authorization files are not found
        // 5: Recognition engine is failed to load templates
        // 6: Chip reader initialization is failed
    }
    return result; // Return the result to the caller (which will be Flutter via platform channel)
}

void SinosecuScanner::releaseScanner() {
    std::cout << "Releasing Sinosecu SDK..." << std::endl;
    FreeIDCard(); // [cite: 31]
    std::cout << "Sinosecu SDK released." << std::endl;
}
