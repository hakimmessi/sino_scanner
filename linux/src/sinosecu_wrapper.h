#ifndef SINOSECU_WRAPPER_H
#define SINOSECU_WRAPPER_H

#include <string>

std::wstring string_to_wstring(const std::string& str);

extern "C" {
    int InitIDCard(const wchar_t* IpUserID, int nType, const wchar_t* IpDirectory);
    void FreeIDCard();
    int DetectDocument();
    int AutoProcessIDCard(int& nCardType);
}

#include <map>

// this class will wrap the SDK methods
class SinosecuScanner {
public:
    // this constructor is called when the object is created
    SinosecuScanner();
    // this destructor is called when the object is destroyed
    ~SinosecuScanner();

    int initializeScanner(const std::string& userId, int nType, const std::string& sdkDirectory);
    int detectDocumentOnScanner();
    std::map<std::string, int> autoProcessDocument();
    void releaseScanner();
};
#endif
