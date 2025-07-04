#ifndef SINOSECU_WRAPPER_H
#define SINOSECU_WRAPPER_H

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>

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
int GetIDCardName(wchar_t* lpBuffer, int& nBufferLen);

// Device status
int CheckDeviceOnlineEx();

// Configuration functions
int SetConfigByFile(const wchar_t* lpConfigFile);
int SetLanguage(int nLangType);
void SetSaveImageType(int nImageType);
void SetRecogVIZ(bool bRecogVIZ);
void SetRecogDG(int nDG);
int SetRecogChipCardAttribute(int nReadCard);
void ResetIDCardID();
int AddIDCardID(int nMainID, int nSubID[], int nSubIDCount);

// Image saving
int SaveImageEx(const wchar_t* lpFileName, int nType);
}


class SinosecuScanner {
public:
    SinosecuScanner();
    ~SinosecuScanner();

    // Core functionality
    int initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory);
    int detectDocumentOnScanner();
    std::map<std::string, int> autoProcessDocument();
    void releaseScanner();

    // Device and document operations
    int checkDeviceStatus();
    std::map<std::string, std::string> getDocumentFields(int attribute = 1); // 1 = OCR page data
    int loadConfiguration(const std::string& configPath);
    bool saveImages(const std::string& basePath, int imageTypes = 0x1F); // Save all image types
    bool configureDocumentTypes();
    int waitForDocumentDetection(int timeoutSeconds = 30);
    std::string getDocumentName();

    // Complete scanning workflows
    std::map<std::string, std::string> scanDocumentComplete(int timeoutSeconds = 20);
    std::map<std::string, std::string> scanDocumentCompleteWithDebug(int timeoutSeconds = 20, bool enableDebug = false);

    // Formatted data extraction (matches GUI display format)
    std::map<std::string, std::string> getFormattedPassportData();

    // Field validation methods
    bool isValidPassportNumber(const std::string& passportNum);
    bool isValidDate(const std::string& dateStr);
    bool isValidMRZ(const std::string& mrzLine);
    std::string getValidatedFieldValue(int attribute, int index, const std::string& fieldName);

    // Debug and utility methods
    void debugAllAvailableFields(int attribute);
    std::string formatDate(const std::string& dateStr);

    // Error handling
    std::string getLastError() const;

    // Return values for error handling
    static constexpr int SUCCESS = 0;
    static constexpr int ERROR_INIT = -1;
    static constexpr int ERROR_DETECT = -2;
    static constexpr int ERROR_PROCESS = -3;
    static constexpr int ERROR_DEVICE = -4;
    static constexpr int ERROR_CONFIG = -5;
    static constexpr int ERROR_TIMEOUT = -100;

private:
    bool isInitialized;
    std::string lastError;
    std::string sdkPath;

    // Processing and error handling
    std::map<std::string, std::string> handleProcessingResult(int processResult, int cardType);
    std::string getProcessingErrorMessage(int errorCode);

    // Helper methods
    void setLastError(const std::string& error);
    std::string getFieldValue(int attribute, int index);
    bool validateInitialization();
};

#endif