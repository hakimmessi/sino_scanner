#include "src/sinosecu_wrapper.h"
#include <iostream>
#include <locale>
#include <codecvt>

std::wstring string_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

SinosecuScanner::SinosecuScanner() : isInitialized(false) {}

SinosecuScanner::~SinosecuScanner() {
    releaseScanner();
}

int SinosecuScanner::initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory) {
    if (isInitialized) {
        releaseScanner();
    }

    std::wstring wUserId = string_to_wstring(userId);
    std::wstring wSdkDirectory = string_to_wstring(sdkDirectory);

    int result = InitIDCard(wUserId.c_str(), nType, wSdkDirectory.c_str());
    if (result == SUCCESS) {
        isInitialized = true;
    }
    return result;
}

void SinosecuScanner::releaseScanner() {
    if (isInitialized) {
        FreeIDCard();
        isInitialized = false;
    }
}

int SinosecuScanner::detectDocumentOnScanner() {
    if (!isInitialized) {
        return ERROR_INIT;
    }
    return DetectDocument();
}

std::map<std::string, int> SinosecuScanner::autoProcessDocument() {
    std::map<std::string, int> result;
    
    if (!isInitialized) {
        result["status"] = ERROR_INIT;
        return result;
    }

    int cardType = 0;
    int processResult = AutoProcessIDCard(cardType);
    
    result["status"] = processResult;
    result["cardType"] = cardType;
    
    return result;
}
