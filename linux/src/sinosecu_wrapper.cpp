#include "sinosecu_wrapper.h"
#include "png_wrapper.h"
#include <iostream>
#include <locale>
#include <codecvt>
#include <filesystem>

std::wstring string_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

std::string wstring_to_string(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

SinosecuScanner::SinosecuScanner() : isInitialized(false) {}

SinosecuScanner::~SinosecuScanner() {
    releaseScanner();
}

void SinosecuScanner::setLastError(const std::string& error) {
    lastError = error;
    std::cerr << "SinosecuScanner Error: " << error << std::endl;
}

std::string SinosecuScanner::getLastError() const {
    return lastError;
}

bool SinosecuScanner::validateInitialization() {
    if (!isInitialized) {
        setLastError("Scanner not initialized");
        return false;
    }
    return true;
}

int SinosecuScanner::initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory) {
    std::cout << "=== SinosecuScanner::initializeScanner ===" << std::endl;
    std::cout << "Architecture: " <<
              #ifdef ARM_BUILD
              "ARM64"
              #else
              "x86/x64"
              #endif
              << std::endl;

    // First check if directory exists
    if (!std::filesystem::exists(sdkDirectory)) {
        setLastError("SDK directory does not exist: " + sdkDirectory);
        return ERROR_INIT;
    }

    // Check for required files
    std::string libPath = sdkDirectory + "/libIDCard.so";
    if (!std::filesystem::exists(libPath)) {
        setLastError("libIDCard.so not found in: " + sdkDirectory);
        return ERROR_INIT;
    }

    std::cout << "SDK directory verified: " << sdkDirectory << std::endl;
    std::cout << "libIDCard.so found at: " << libPath << std::endl;

    if (isInitialized) {
        std::cout << "Scanner already initialized, releasing first..." << std::endl;
        releaseScanner();
    }

    std::wstring wUserId = string_to_wstring(userId);
    std::wstring wSdkDirectory = string_to_wstring(sdkDirectory);

    std::cout << "Calling InitIDCard with:" << std::endl;
    std::cout << "  UserID: " << userId << std::endl;
    std::cout << "  nType: " << nType << std::endl;
    std::cout << "  Directory: " << sdkDirectory << std::endl;

    int result;
    try {
        result = InitIDCard(wUserId.c_str(), nType, wSdkDirectory.c_str());
        std::cout << "InitIDCard returned: " << result << std::endl;
    } catch (const std::exception& e) {
        setLastError("Exception during InitIDCard: " + std::string(e.what()));
        return ERROR_INIT;
    } catch (...) {
        setLastError("Unknown exception during InitIDCard - possible library compatibility issue");
        return ERROR_INIT;
    }

    switch (result) {
        case 0:
            isInitialized = true;
            sdkPath = sdkDirectory;
            std::cout << "✓ SDK initialized successfully!" << std::endl;
            break;
        case 1:
            setLastError("Authorization ID is incorrect - check your license");
            break;
        case 2:
            setLastError("Device initialization failed - check device connection");
            break;
        case 3:
            setLastError("Recognition engine initialization failed - check SDK files");
            break;
        case 4:
            setLastError("Authorization files not found - check license files in SDK directory");
            break;
        case 5:
            setLastError("Recognition engine failed to load templates - check template files");
            break;
        case 6:
            setLastError("Chip reader initialization failed - check device drivers");
            break;
        default:
            setLastError("Unknown initialization error: " + std::to_string(result));
            break;
    }

    std::cout << "=== initializeScanner complete ===" << std::endl;
    return result;
}

void SinosecuScanner::releaseScanner() {
    if (isInitialized) {
        try {
            FreeIDCard();
            std::cout << "SDK released successfully" << std::endl;
        } catch (const std::exception& e) {
            setLastError("Error releasing SDK: " + std::string(e.what()));
        }
        isInitialized = false;
    }
}

int SinosecuScanner::checkDeviceStatus() {
    if (!validateInitialization()) {
        return ERROR_INIT;
    }

    try {
        return CheckDeviceOnlineEx();
    } catch (const std::exception& e) {
        setLastError("Error checking device status: " + std::string(e.what()));
        return ERROR_DEVICE;
    }
}

int SinosecuScanner::detectDocumentOnScanner() {
    if (!validateInitialization()) {
        return ERROR_INIT;
    }

    // First check device status
    int deviceStatus = checkDeviceStatus();
    if (deviceStatus != 1) {
        setLastError("Device not ready. Status: " + std::to_string(deviceStatus));
        return ERROR_DEVICE;
    }

    try {
        int result = DetectDocument();
        std::cout << "Document detection result: " << result << std::endl;
        return result;
    } catch (const std::exception& e) {
        setLastError("Error detecting document: " + std::string(e.what()));
        return ERROR_DETECT;
    }
}

std::map<std::string, int> SinosecuScanner::autoProcessDocument() {
    std::map<std::string, int> result;

    if (!validateInitialization()) {
        result["status"] = ERROR_INIT;
        return result;
    }

    try {
        int cardType = 0;
        int processResult = AutoProcessIDCard(cardType);

        result["status"] = processResult;
        result["cardType"] = cardType;

        std::cout << "Auto process result: " << processResult << ", Card type: " << cardType << std::endl;

        // Interpret results based on documentation
        if (processResult > 0) {
            std::cout << "Document processed successfully. Main type: " << processResult << std::endl;
        } else {
            switch (processResult) {
                case -1:
                    setLastError("Did not set up valid document types for automatic classification");
                    break;
                case -2:
                    setLastError("Image capturing failed");
                    break;
                case -3:
                    setLastError("Image cutting failed");
                    break;
                case -4:
                    setLastError("Classification failed - no matched template");
                    break;
                case -5:
                    setLastError("Classification failed - no valid document types set");
                    break;
                case -6:
                    setLastError("Classification failed - recognition rejected");
                    break;
                case -7:
                    setLastError("Recognition failed");
                    break;
                case -8:
                    setLastError("Chip reading failed but page recognition successful");
                    break;
                case -9:
                    setLastError("Chip reading successful but page recognition failed");
                    break;
                case -10:
                    setLastError("Both page recognition and chip reading failed");
                    break;
                default:
                    setLastError("Unknown processing error: " + std::to_string(processResult));
                    break;
            }
        }

    } catch (const std::exception& e) {
        setLastError("Exception during document processing: " + std::string(e.what()));
        result["status"] = ERROR_PROCESS;
        result["cardType"] = 0;
    }

    return result;
}

int SinosecuScanner::loadConfiguration(const std::string& configPath) {
    if (!validateInitialization()) {
        return ERROR_INIT;
    }

    if (!std::filesystem::exists(configPath)) {
        setLastError("Configuration file does not exist: " + configPath);
        return ERROR_CONFIG;
    }

    try {
        std::wstring wConfigPath = string_to_wstring(configPath);
        int result = SetConfigByFile(wConfigPath.c_str());

        if (result == 0) {
            std::cout << "Configuration loaded successfully from: " << configPath << std::endl;
        } else {
            setLastError("Failed to load configuration. Result: " + std::to_string(result));
        }

        return result;
    } catch (const std::exception& e) {
        setLastError("Exception loading configuration: " + std::string(e.what()));
        return ERROR_CONFIG;
    }
}

std::string SinosecuScanner::getFieldValue(int attribute, int index) {
    const int bufferSize = 1024;
    wchar_t buffer[bufferSize];
    int actualSize = bufferSize;

    try {
        int result = GetRecogResultEx(attribute, index, buffer, actualSize);
        if (result == 0) {
            return wstring_to_string(std::wstring(buffer, actualSize));
        }
    } catch (const std::exception& e) {
        setLastError("Exception getting field value: " + std::string(e.what()));
    }

    return "";
}

std::map<std::string, std::string> SinosecuScanner::getDocumentFields(int attribute) {
    std::map<std::string, std::string> fields;

    if (!validateInitialization()) {
        return fields;
    }

    // Common field indices for passport/ID documents
    std::vector<std::pair<int, std::string>> fieldMap = {
            {1, "name"},
            {2, "gender"},
            {3, "nationality"},
            {4, "birthday"},
            {5, "address"},
            {6, "id_number"},
            {7, "issue_authority"},
            {8, "issue_date"},
            {9, "expiry_date"}
    };

    for (const auto& field : fieldMap) {
        std::string value = getFieldValue(attribute, field.first);
        if (!value.empty()) {
            fields[field.second] = value;
        }
    }

    return fields;
}

bool SinosecuScanner::saveImages(const std::string& basePath, int imageTypes) {
    if (!validateInitialization()) {
        return false;
    }

    try {
        std::string imagePath = basePath + ".jpg";
        std::wstring wImagePath = string_to_wstring(imagePath);

        int result = SaveImageEx(wImagePath.c_str(), imageTypes);

        if (result == 0) {
            std::cout << "Images saved successfully to: " << imagePath << std::endl;
            return true;
        } else {
            setLastError("Failed to save images. Result: " + std::to_string(result));
            return false;
        }
    } catch (const std::exception& e) {
        setLastError("Exception saving images: " + std::string(e.what()));
        return false;
    }
}