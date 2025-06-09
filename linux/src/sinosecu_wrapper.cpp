#include "sinosecu_wrapper.h"
#include "png_wrapper.h"
#include <iostream>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <thread>
#include <chrono>

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
    std::cout << "Architecture: "
                 #ifdef __aarch64__
                 "ARM64"
                 #elif defined(__x86_64__) || defined(_M_X64)
                 "x86/x64"
                 #else
                 "Unknown"
              #endif
              << std::endl;

    // Check if directory exists
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

            // Configure scanner for common document types after successful initialization
            if (!configureDocumentTypes()) {
                setLastError("Failed to configure document types");
                releaseScanner();
                return ERROR_CONFIG;
            }
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

bool SinosecuScanner::configureDocumentTypes() {
    try {
        // Clear any existing document types
        ResetIDCardID();

        // Set language to English
        SetLanguage(1);

        // Configure common document types for recognition
        // Based on the SDK documentation, these are common document main IDs:

        // Chinese ID Card (front and back)
        int subIDs[] = {0}; // 0 means all sub-types
        AddIDCardID(2, subIDs, 1);  // Resident identification card-photo page
        AddIDCardID(3, subIDs, 1);  // Resident identification card-issuing authority page

        // Driver's License
        AddIDCardID(5, subIDs, 1);  // Vehicle drivers license

        // Passport
        AddIDCardID(13, subIDs, 1); // Passport

        // Visa
        AddIDCardID(12, subIDs, 1); // Visa

        // Set image capture options
        SetSaveImageType(0x1F); // Capture all image types (white, IR, UV, portraits)

        // Enable page recognition
        SetRecogVIZ(true);

        // Enable chip reading if available
        SetRecogDG(0xFFFF); // Enable all data groups

        std::cout << "Document types and settings configured successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        setLastError("Exception configuring document types: " + std::string(e.what()));
        return false;
    }
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
        int status = CheckDeviceOnlineEx();
        std::cout << "Device status: " << status << std::endl;

        switch(status) {
            case 1:
                std::cout << "Device connected and initialized" << std::endl;
                break;
            case 2:
                std::cout << "Device lost connection" << std::endl;
                break;
            case 3:
                std::cout << "Device lost connection - need re-initialization" << std::endl;
                break;
            default:
                std::cout << "Unknown device status: " << status << std::endl;
                break;
        }

        return status;
    } catch (const std::exception& e) {
        setLastError("Error checking device status: " + std::string(e.what()));
        return ERROR_DEVICE;
    }
}

int SinosecuScanner::detectDocumentOnScanner() {
    if (!validateInitialization()) {
        return ERROR_INIT;
    }

    // Check device status first
    int deviceStatus = checkDeviceStatus();
    if (deviceStatus == 3) {
        setLastError("Device needs re-initialization");
        return ERROR_DEVICE;
    } else if (deviceStatus == 2) {
        setLastError("Device not connected");
        return ERROR_DEVICE;
    }

    try {
        int result = DetectDocument();

        switch(result) {
            case -1:
                std::cout << "Core engine not initialized" << std::endl;
                break;
            case 0:
                std::cout << "No document detected" << std::endl;
                break;
            case 1:
                std::cout << "Document detected and placed" << std::endl;
                break;
            case 2:
                std::cout << "Document was taken out" << std::endl;
                break;
            case 3:
                std::cout << "Mobile phone barcode detected (AR/KR series)" << std::endl;
                break;
            default:
                std::cout << "Unknown detection result: " << result << std::endl;
                break;
        }

        return result;
    } catch (const std::exception& e) {
        setLastError("Error detecting document: " + std::string(e.what()));
        return ERROR_DETECT;
    }
}

int SinosecuScanner::waitForDocumentDetection(int timeoutSeconds) {
    if (!validateInitialization()) {
        return ERROR_INIT;
    }

    std::cout << "Waiting for document detection (timeout: " << timeoutSeconds << "s)..." << std::endl;

    auto startTime = std::chrono::steady_clock::now();
    auto timeoutDuration = std::chrono::seconds(timeoutSeconds);

    while (true) {
        int result = detectDocumentOnScanner();

        if (result == 1) { // Document detected
            return result;
        }

        if (result < 0) { // Error occurred
            return result;
        }

        // Check timeout
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime - startTime > timeoutDuration) {
            setLastError("Document detection timeout");
            return ERROR_TIMEOUT;
        }

        // Wait a bit before next check
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

        // Interpret cardType flags
        if (cardType & 1) std::cout << "  → Document has chip" << std::endl;
        if (cardType & 2) std::cout << "  → Document has no chip" << std::endl;
        if (cardType & 4) std::cout << "  → Document has barcode" << std::endl;

        // Interpret results based on documentation
        if (processResult > 0) {
            std::cout << "✓ Document processed successfully. Main type: " << processResult << std::endl;

            // Get document name
            std::string docName = getDocumentName();
            if (!docName.empty()) {
                std::cout << "  → Document type: " << docName << std::endl;
            }

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

std::string SinosecuScanner::getDocumentName() {
    const int bufferSize = 256;
    wchar_t buffer[bufferSize];
    int actualSize = bufferSize;

    try {
        int result = GetIDCardName(buffer, actualSize);
        if (result == 0) {
            return wstring_to_string(std::wstring(buffer, actualSize));
        }
    } catch (const std::exception& e) {
        setLastError("Exception getting document name: " + std::string(e.what()));
    }

    return "";
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
        } else if (result == 1) {
            // Buffer too small, try again with larger buffer
            std::vector<wchar_t> largerBuffer(actualSize);
            result = GetRecogResultEx(attribute, index, largerBuffer.data(), actualSize);
            if (result == 0) {
                return wstring_to_string(std::wstring(largerBuffer.data(), actualSize));
            }
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

    // Extended field mapping for common documents
    std::vector<std::pair<int, std::string>> fieldMap = {
            {1, "name"},
            {2, "gender"},
            {3, "nationality"},
            {4, "birthday"},
            {5, "address"},
            {6, "id_number"},
            {7, "issue_authority"},
            {8, "issue_date"},
            {9, "expiry_date"},
            {10, "passport_number"},
            {11, "place_of_birth"},
            {12, "place_of_issue"}
    };

    std::cout << "Extracting fields for attribute " << attribute << ":" << std::endl;

    for (const auto& field : fieldMap) {
        std::string value = getFieldValue(attribute, field.first);
        if (!value.empty()) {
            fields[field.second] = value;
            std::cout << "  " << field.second << ": " << value << std::endl;
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

            // Interpret the result bits for partial success
            if (result > 0) {
                std::cout << "Partial image save failure:" << std::endl;
                if (result & 1) std::cout << "  White image save failed" << std::endl;
                if (result & 2) std::cout << "  IR image save failed" << std::endl;
                if (result & 4) std::cout << "  UV image save failed" << std::endl;
                if (result & 8) std::cout << "  Page portrait save failed" << std::endl;
                if (result & 16) std::cout << "  Chip portrait save failed" << std::endl;
            }

            return false;
        }
    } catch (const std::exception& e) {
        setLastError("Exception saving images: " + std::string(e.what()));
        return false;
    }
}

// Additional utility method for complete document scanning workflow
std::map<std::string, std::string> SinosecuScanner::scanDocumentComplete(int timeoutSeconds) {
    std::map<std::string, std::string> result;

    if (!validateInitialization()) {
        result["error"] = "Scanner not initialized";
        return result;
    }

    std::cout << "\n=== Starting Complete Document Scan ===" << std::endl;

    // Step 1: Wait for document detection
    int detectionResult = waitForDocumentDetection(timeoutSeconds);
    if (detectionResult != 1) {
        result["error"] = "Document detection failed: " + std::to_string(detectionResult);
        return result;
    }

    // Step 2: Process the document
    auto processResult = autoProcessDocument();
    if (processResult["status"] <= 0) {
        result["error"] = "Document processing failed: " + std::to_string(processResult["status"]);
        result["error_detail"] = getLastError();
        return result;
    }

    // Step 3: Extract fields
    auto ocrFields = getDocumentFields(1); // OCR fields
    auto chipFields = getDocumentFields(0); // Chip fields

    // Combine results
    result["document_type"] = getDocumentName();
    result["main_type"] = std::to_string(processResult["status"]);
    result["card_type"] = std::to_string(processResult["cardType"]);

    // Add OCR fields
    for (const auto& field : ocrFields) {
        result["ocr_" + field.first] = field.second;
    }

    // Add chip fields
    for (const auto& field : chipFields) {
        result["chip_" + field.first] = field.second;
    }

    std::cout << "=== Document Scan Complete ===" << std::endl;
    return result;
}