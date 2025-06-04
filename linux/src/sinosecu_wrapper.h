#ifndef SINOSECU_WRAPPER_H
#define SINOSECU_WRAPPER_H

#include <string>
#include <map>
#include <vector>

// Forward declaration
class PngWrapper;

// Utility function to convert std::string to std::wstring
std::wstring string_to_wstring(const std::string& str);
std::string wstring_to_string(const std::wstring& wstr);

// Include all necessary SDK functions based on the documentation
extern "C" {
// Core functions
int InitIDCard(const wchar_t* lpUserID, int nType, const wchar_t* lpDirectory);
void FreeIDCard();

// Device detection and processing
int DetectDocument();
int AutoProcessIDCard(int& nCardType);

// Result retrieval functions
int GetFieldNameEx(int nAttribute, int nIndex, wchar_t* lpBuffer, int& nBufferLen);
int GetRecogResultEx(int nAttribute, int nIndex, wchar_t* lpBuffer, int& nBufferLen);
int GetResultTypeEx(int nAttribute, int nIndex);
int GetFieldConfEx(int nAttribute, int nIndex);

// Device status
int CheckDeviceOnlineEx();

// Configuration
int SetConfigByFile(const wchar_t* lpConfigFile);

// Image saving
int SaveImageEx(const wchar_t* lpFileName, int nType);
}

// Enhanced wrapper class
class SinosecuScanner {
public:
    SinosecuScanner();
    ~SinosecuScanner();

    // Core functionality
    int initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory);
    int detectDocumentOnScanner();
    std::map<std::string, int> autoProcessDocument();
    void releaseScanner();

    // Enhanced functionality
    int checkDeviceStatus();
    std::map<std::string, std::string> getDocumentFields(int attribute = 1); // 1 = OCR page data
    int loadConfiguration(const std::string& configPath);
    bool saveImages(const std::string& basePath, int imageTypes = 0x1F); // Save all image types

    // Error handling
    std::string getLastError() const;

    // Return values for error handling
    static constexpr int SUCCESS = 0;
    static constexpr int ERROR_INIT = -1;
    static constexpr int ERROR_DETECT = -2;
    static constexpr int ERROR_PROCESS = -3;
    static constexpr int ERROR_DEVICE = -4;
    static constexpr int ERROR_CONFIG = -5;

private:
    bool isInitialized;
    std::string lastError;
    std::string sdkPath;

    // Helper methods
    void setLastError(const std::string& error);
    std::string getFieldValue(int attribute, int index);
    bool validateInitialization();
};

#endif