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
    if (wstr.empty()) {
        return "";
    }

    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
        return conv.to_bytes(wstr);
    } catch (const std::exception& e) {
        std::cerr << "String conversion error: " << e.what() << std::endl;

        // Fallback: manual conversion for basic ASCII
        std::string result;
        for (wchar_t wc : wstr) {
            if (wc < 128) {
                result += static_cast<char>(wc);
            } else {
                result += '?';
            }
        }
        return result;
    }
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
            std::cout << "SDK initialized successfully!" << std::endl;

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
        //AddIDCardID(2, subIDs, 1);  // Resident identification card-photo page
       // AddIDCardID(3, subIDs, 1);  // Resident identification card-issuing authority page

        // Driver's License
       // AddIDCardID(5, subIDs, 1);  // Vehicle drivers license

        // Passport
        AddIDCardID(13, subIDs, 1); // Passport

        // Visa
       // AddIDCardID(12, subIDs, 1); // Visa

        // Set image capture options
        SetSaveImageType(0x1F); // Capture all image types (white, IR, UV, portraits)

        // Enable page recognition
        SetRecogVIZ(true);

        std::cout << "Configuring chip reading..." << std::endl;
        try {
            // Enable chip reading for documents that have chips
            int chipResult = SetRecogChipCardAttribute(1); // 1 = enable chip reading

            switch(chipResult) {
                case 0:
                    std::cout << "Chip reading enabled successfully" << std::endl;

                    // Configure which data groups to read from the chip
                    SetRecogDG(1); // Enable  (common passport data groups)
                    //std::cout << "Passport chip data groups configured (DG1-DG12)" << std::endl;
                    break;

                case 1:
                    std::cout << "Chip reading setup failed - device not initialized" << std::endl;
                    break;

                case 2:
                    std::cout << "Chip reading not supported by this device" << std::endl;
                    std::cout << "Continuing with OCR-only mode..." << std::endl;
                    break;

                default:
                    std::cout << "Chip reading setup returned: " << chipResult << std::endl;
                    break;
            }

        } catch (const std::exception& e) {
            std::cout << "Chip configuration exception: " << e.what() << std::endl;
            std::cout << "Continuing with OCR-only mode..." << std::endl;
        }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
        // Initialize buffer to prevent issues
        std::wmemset(buffer, 0, bufferSize);

        int result = GetIDCardName(buffer, actualSize);
        if (result == 0 && actualSize > 0) {
            // Ensure null termination
            if (actualSize >= bufferSize) {
                actualSize = bufferSize - 1;
            }
            buffer[actualSize] = L'\0';

            std::wstring wstr(buffer, actualSize);
            return wstring_to_string(wstr);
        } else {
            std::cout << "GetIDCardName failed or returned empty. Result: " << result << ", Size: " << actualSize << std::endl;
            return "";
        }
    } catch (const std::exception& e) {
        setLastError("Exception getting document name: " + std::string(e.what()));
        return "";
    }
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
        // Initialize buffer
        std::wmemset(buffer, 0, bufferSize);

        int result = GetRecogResultEx(attribute, index, buffer, actualSize);

        if (result == 0 && actualSize > 0) {
            // Ensure proper termination
            if (actualSize >= bufferSize) {
                actualSize = bufferSize - 1;
            }
            buffer[actualSize] = L'\0';

            std::wstring wstr(buffer, actualSize);
            std::string converted = wstring_to_string(wstr);

            // Debug output
            std::cout << "  Field[" << attribute << "][" << index << "]: '" << converted << "' (size: " << actualSize << ")" << std::endl;

            return converted;
        } else if (result == 1) {
            // Buffer too small, try again with larger buffer
            std::cout << "  Buffer too small for field[" << attribute << "][" << index << "], need: " << actualSize << std::endl;

            if (actualSize > 0 && actualSize < 10000) { // Sanity check
                std::vector<wchar_t> largerBuffer(actualSize + 1);
                std::wmemset(largerBuffer.data(), 0, actualSize + 1);

                int newSize = actualSize;
                result = GetRecogResultEx(attribute, index, largerBuffer.data(), newSize);
                if (result == 0) {
                    largerBuffer[newSize] = L'\0';
                    std::wstring wstr(largerBuffer.data(), newSize);
                    return wstring_to_string(wstr);
                }
            }
        } else {
            std::cout << "  Field[" << attribute << "][" << index << "] not available. Result: " << result << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "  Exception getting field[" << attribute << "][" << index << "]: " << e.what() << std::endl;
    }

    return "";
}

bool SinosecuScanner::isValidPassportNumber(const std::string& passportNum) {
    // Basic passport number validation
    if (passportNum.length() < 6 || passportNum.length() > 12) {
        return false;
    }

    // Check if contains only alphanumeric characters
    return std::all_of(passportNum.begin(), passportNum.end(),
                       [](char c) { return std::isalnum(c); });
}

bool SinosecuScanner::isValidDate(const std::string& dateStr) {
    // Check if date is in YYYYMMDD format
    if (dateStr.length() != 8) {
        return false;
    }

    // Check if all digits
    if (!std::all_of(dateStr.begin(), dateStr.end(), ::isdigit)) {
        return false;
    }

    // Basic range validation
    int year = std::stoi(dateStr.substr(0, 4));
    int month = std::stoi(dateStr.substr(4, 2));
    int day = std::stoi(dateStr.substr(6, 2));

    return (year >= 1900 && year <= 2100 &&
            month >= 1 && month <= 12 &&
            day >= 1 && day <= 31);
}

bool SinosecuScanner::isValidMRZ(const std::string& mrzLine) {
    // MRZ lines should be exactly 44 characters
    if (mrzLine.length() != 44) {
        return false;
    }

    // Should contain only uppercase letters, digits, and '<'
    return std::all_of(mrzLine.begin(), mrzLine.end(),
                       [](char c) {
                           return std::isupper(c) || std::isdigit(c) || c == '<';
                       });
}

// Enhanced field extraction with validation
std::string SinosecuScanner::getValidatedFieldValue(int attribute, int index, const std::string& fieldName) {
    std::string value = getFieldValue(attribute, index);

    if (value.empty()) {
        return value;
    }

    // Apply field-specific validation
    if (fieldName == "passport_number_mrz" || fieldName == "passport_number_direct") {
        if (!isValidPassportNumber(value)) {
            std::cout << "Warning: Invalid passport number format: " << value << std::endl;
        }
    } else if (fieldName == "date_of_birth" || fieldName == "date_of_expiry" || fieldName == "date_of_issue") {
        if (!isValidDate(value)) {
            std::cout << "Warning: Invalid date format: " << value << std::endl;
        }
    } else if (fieldName == "mrz_line_1" || fieldName == "mrz_line_2") {
        if (!isValidMRZ(value)) {
            std::cout << "Warning: Invalid MRZ format: " << value << std::endl;
        }
    } else if (fieldName == "gender") {
        if (value != "M" && value != "F" && value != "X") {
            std::cout << "Warning: Unexpected gender value: " << value << std::endl;
        }
    }

    return value;
}

std::map<std::string, std::string> SinosecuScanner::getDocumentFields(int attribute) {
    std::map<std::string, std::string> fields;

    if (!validateInitialization()) {
        return fields;
    }

    std::vector<std::pair<int, std::string>> passportFieldMap = {
            // Core MRZ fields (these exactly match the SDK documentation)
            {0,  "passport_type"},           // "P" - Passport type from MRZ
            {1,  "passport_number_mrz"},     // Passport number from MRZ
            {2,  "domestic_name"},           // Name in local language/script
            {3,  "english_name"},            // Full English name
            {4,  "gender"},                  // "M" or "F"
            {5,  "date_of_birth"},           // Birth date in YYYYMMDD format
            {6,  "date_of_expiry"},          // Expiry date in YYYYMMDD format
            {7,  "issuing_country_code"},    // 3-letter country code
            {8,  "english_surname"},         // Surname only
            {9,  "english_first_name"},      // First name(s)
            {10, "mrz_line_1"},              // First MRZ line (44 characters)
            {11, "mrz_line_2"},              // Second MRZ line (44 characters)
            {12, "nationality_code"},        // Holder nationality code from MRZ

            // Additional passport fields
            {13, "passport_number_direct"},  // Passport number from direct OCR (visual zone)
            {14, "place_of_birth"},          // Birth place
            {15, "place_of_issue"},          // Issue place
            {16, "date_of_issue"},           // Issue date
            {17, "rfid_mrz"},                // MRZ data from RFID chip
            {18, "ocr_mrz"},                 // MRZ data from OCR
            {21, "national_id_number"},      // National ID number
            {23, "gender_ocr"},              // Gender from OCR
            {24, "nationality_code_ocr"},    // Nationality from OCR
            {25, "id_card_number_ocr"},      // ID card number from OCR
            {26, "birth_date_ocr"},          // Birth date from OCR
            {27, "valid_until_ocr"},         // Valid until from OCR
            {28, "issuing_authority_ocr"},   // Issuing authority from OCR
            {29, "domestic_surname"},        // Domestic surname
            {30, "domestic_first_name"}      // Domestic first name
    };

    std::cout << "Extracting " << (attribute == 0 ? "CHIP" : "OCR") << " fields:" << std::endl;

    for (const auto& field : passportFieldMap) {
        std::string value = getFieldValue(attribute, field.first);
        if (!value.empty() && value != " " && value.length() > 0) {
            // Clean up the value
            value.erase(0, value.find_first_not_of(" \t\n\r")); // trim left
            value.erase(value.find_last_not_of(" \t\n\r") + 1); // trim right

            if (!value.empty()) {
                fields[field.second] = value;
                std::cout << "  ✓ " << field.second << ": '" << value << "'" << std::endl;
            }
        }
    }

    if (fields.empty()) {
        std::cout << "No fields extracted for attribute " << attribute << std::endl;
    }

    return fields;
}


std::map<std::string, std::string> SinosecuScanner::getFormattedPassportData() {
    std::map<std::string, std::string> formattedData;

    if (!validateInitialization()) {
        return formattedData;
    }

    // Get OCR fields (attribute = 1) - this is what you'll primarily use
    auto ocrFields = getDocumentFields(1);


    if (ocrFields.count("passport_number_mrz")) {
        formattedData["passport_number"] = "The passport number from MRZ " + ocrFields["passport_number_mrz"];
    }

    if (ocrFields.count("english_name")) {
        formattedData["english_name"] = ocrFields["english_name"];
    }

    if (ocrFields.count("gender")) {
        formattedData["sex"] = ocrFields["gender"];
    }

    if (ocrFields.count("date_of_birth")) {
        formattedData["date_of_birth"] = formatDate(ocrFields["date_of_birth"]);
    }

    if (ocrFields.count("date_of_expiry")) {
        formattedData["date_of_expiry"] = formatDate(ocrFields["date_of_expiry"]);
    }

    if (ocrFields.count("issuing_country_code")) {
        formattedData["issuing_country_code"] = ocrFields["issuing_country_code"];
    }

    if (ocrFields.count("english_surname")) {
        formattedData["english_surname"] = ocrFields["english_surname"];
    }

    if (ocrFields.count("english_first_name")) {
        formattedData["english_first_name"] = ocrFields["english_first_name"];
    }

    return formattedData;
}

// Helper to format dates from YYYYMMDD to YYYY-MM-DD
std::string SinosecuScanner::formatDate(const std::string& dateStr) {
    if (dateStr.length() == 8 && std::all_of(dateStr.begin(), dateStr.end(), ::isdigit)) {
        std::string year = dateStr.substr(0, 4);
        std::string month = dateStr.substr(4, 2);
        std::string day = dateStr.substr(6, 2);
        return year + "-" + month + "-" + day;
    }
    return dateStr;
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

std::map<std::string, std::string> SinosecuScanner::handleProcessingResult(int processResult, int cardType) {
    std::map<std::string, std::string> result;

    if (processResult > 0) {
        // Success - document recognized
        result["status"] = "success";
        result["main_type"] = std::to_string(processResult);
        result["card_type"] = std::to_string(cardType);

        // Get document name
        std::string docName = getDocumentName();
        if (!docName.empty()) {
            result["document_type"] = docName;
        }

    } else if (processResult == -8) {
        // Chip reading failed but OCR succeeded
        std::cout << "Chip reading failed, but OCR was successful" << std::endl;
        std::cout << "This is common - continuing with OCR data only..." << std::endl;

        result["status"] = "partial_success";
        result["main_type"] = "unknown"; // We don't know the main type due to chip failure
        result["card_type"] = std::to_string(cardType);
        result["warning"] = "Chip reading failed - using OCR data only";

        // Still try to get document name and OCR fields
        std::string docName = getDocumentName();
        if (!docName.empty()) {
            result["document_type"] = docName;
        }

    } else if (processResult == -9) {
        // Chip reading succeeded but OCR failed
        std::cout << "OCR failed, but chip reading was successful" << std::endl;

        result["status"] = "partial_success";
        result["main_type"] = std::to_string(abs(processResult)); // Use absolute value
        result["card_type"] = std::to_string(cardType);
        result["warning"] = "OCR failed - using chip data only";

    } else {
        // Complete failure
        result["error"] = getProcessingErrorMessage(processResult);
        result["status"] = "error";
        result["main_type"] = std::to_string(processResult);
        result["card_type"] = std::to_string(cardType);
    }

    return result;
}

void SinosecuScanner::debugAllAvailableFields(int attribute) {
    std::cout << "\n=== DEBUG: Scanning all fields for attribute " << attribute << " ===" << std::endl;

    // Try indices 0-50 to see what's available
    for (int i = 0; i <= 50; i++) {
        std::string value = getFieldValue(attribute, i);
        if (!value.empty() && value != " ") {
            std::cout << "Index " << i << ": '" << value << "'" << std::endl;

            // Also try to get the field name
            const int bufferSize = 256;
            wchar_t nameBuffer[bufferSize];
            int nameSize = bufferSize;
            std::wmemset(nameBuffer, 0, bufferSize);

            int nameResult = GetFieldNameEx(attribute, i, nameBuffer, nameSize);
            if (nameResult == 0 && nameSize > 0) {
                nameBuffer[std::min(nameSize, bufferSize - 1)] = L'\0';
                std::wstring wFieldName(nameBuffer, nameSize);
                std::string fieldName = wstring_to_string(wFieldName);
                std::cout << "  → Field name: '" << fieldName << "'" << std::endl;
            }
        }
    }

    std::cout << "=== End debug scan ===" << std::endl;
}


// utility method for complete document scanning workflow
std::map<std::string, std::string> SinosecuScanner::scanDocumentComplete(int timeoutSeconds) {
    std::map<std::string, std::string> result;

    if (!validateInitialization()) {
        result["error"] = "Scanner not initialized";
        return result;
    }

    std::cout << "\n=== Starting Complete Document Scan ===" << std::endl;

    // Wait for document detection
    int detectionResult = waitForDocumentDetection(timeoutSeconds);
    if (detectionResult != 1) {
        result["error"] = "Document detection failed: " + std::to_string(detectionResult);
        return result;
    }

    // Process the document
    auto processResult = autoProcessDocument();
    int status = processResult["status"];
    int cardType = processResult["cardType"];

    std::cout << "Processing result: " << status << ", Card type: " << cardType << std::endl;

    // Handle results with better error tolerance
    if (status > 0 || status == -8 || status == -9) {
        // Success or partial success
        result["status"] = (status > 0) ? "success" : "partial_success";
        result["main_type"] = (status > 0) ? std::to_string(status) : "passport_detected";
        result["card_type"] = std::to_string(cardType);

        if (status == -8) {
            result["warning"] = "Chip reading failed - using OCR data only";
        } else if (status == -9) {
            result["warning"] = "OCR failed - using chip data only";
        }

        // Get document name (with error handling)
        std::string docName = getDocumentName();
        if (!docName.empty()) {
            result["document_type"] = docName;
        } else {
            result["document_type"] = "passport";
        }

    } else {
        // Complete failure
        result["error"] = getProcessingErrorMessage(status);
        result["status"] = "error";
        result["main_type"] = std::to_string(status);
        result["card_type"] = std::to_string(cardType);
        return result; // Don't try to extract fields on complete failure
    }

    // Extract fields with error handling
    try {
        std::cout << "Extracting OCR fields..." << std::endl;
        auto ocrFields = getDocumentFields(1); // OCR fields

        // Add OCR fields with prefix
        for (const auto& field : ocrFields) {
            result["ocr_" + field.first] = field.second;
        }

        std::cout << "Extracting chip fields..." << std::endl;
        auto chipFields = getDocumentFields(0); // Chip fields

        // Add chip fields with prefix
        for (const auto& field : chipFields) {
            result["chip_" + field.first] = field.second;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception during field extraction: " << e.what() << std::endl;
        result["field_extraction_error"] = e.what();
    }

    std::cout << "=== Document Scan Complete ===" << std::endl;

    return result;
}

std::map<std::string, std::string> SinosecuScanner::scanDocumentCompleteWithDebug(int timeoutSeconds, bool enableDebug) {
    auto result = scanDocumentComplete(timeoutSeconds);

    if (enableDebug && (result["status"] == "success" || result["status"] == "partial_success")) {
        std::cout << "\n=== DEBUG MODE ENABLED ===" << std::endl;
        debugAllAvailableFields(1); // OCR fields
        debugAllAvailableFields(0); // Chip fields
    }

    return result;
}

std::string SinosecuScanner::getProcessingErrorMessage(int errorCode) {
    switch (errorCode) {
        case -1: return "No valid document types set for classification";
        case -2: return "Image capturing failed";
        case -3: return "Image cutting failed";
        case -4: return "Classification failed - no matched template";
        case -5: return "Classification failed - no valid document types";
        case -6: return "Classification failed - recognition rejected";
        case -7: return "Recognition failed";
        case -8: return "Chip reading failed but page recognition successful";
        case -9: return "Chip reading successful but page recognition failed";
        case -10: return "Both page recognition and chip reading failed";
        default: return "Unknown processing error: " + std::to_string(errorCode);
    }
}