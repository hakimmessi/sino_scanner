#pragma once
#include <cstdlib>
#define UNICODE
#ifdef UNICODE
typedef wchar_t		TCHAR;
#define _T(x) L ## x
typedef wchar_t*  LPTSTR;
typedef const wchar_t* LPCTSTR;
typedef const wchar_t* LPCWSTR;
#else
#define LPCTSTR const char*
#define LPTSTR char*
#define TCHAR char
#define _T(x) x
#endif
#define WINAPI
class CIDCardRecog
{
public:
	CIDCardRecog(void);
	~CIDCardRecog(void);
	int   (WINAPI *fpInitIDCard)(LPCTSTR lpUserID, int nType,LPCTSTR lpDirectory);
	void  (WINAPI *fpFreeIDCard)();
	int   (WINAPI *fpSetIDCardID)(int nMainID,int nSubID[],int nSubIdCount);
	int   (WINAPI *fpAddIDCardID)(int nMainID,int nSubID[],int nSubIdCount);
	void  (WINAPI *fpResetIDCardID)();
	int   (WINAPI *fpGetSubID)();
	void  (WINAPI *fpGetVersionInfo)(LPCTSTR ,int );
	int   (WINAPI *fpGetGrabSignalType)();
	int   (WINAPI *fpDetectDocument)();
	int   (WINAPI *fpSaveImageEx )(LPCTSTR lpFileName,int nType);
	int   (WINAPI *fpGetDataGroupContent)(int nDGIndex,bool bRawData,unsigned char * lpBuffer,int &len);
	int   (WINAPI *fpGetRecogResultEx)(int nAttribute,int nIndex,LPTSTR lpBuffer,int &nBufferLen);
	int   (WINAPI *fpGetFieldNameEx)(int nAttribute,int nIndex,LPTSTR lpBuffer,int &nBufferLen);
	int   (WINAPI *fpGetFieldConfEx)(int nAttribute,int nIndex);
	int   (WINAPI *fpGetResultTypeEx)(int nAttribute,int nIndex);
	int	  (WINAPI *fpAutoProcessIDCard)(int& nCardType);
	int   (WINAPI *fpSetConfigByFile)(LPCWSTR lpConfigFile);
	int   (WINAPI *fpUTF8CharToWChar)(wchar_t* pwszDest, const char* pszSrc, int nWcharLen);
	int   (WINAPI *fpWCharToUTF8Char)(char* pszDest,const wchar_t* pwszSrc,int nCharLen);
	int	  (WINAPI *fpBuzzerAlarm)(int);
	int   (WINAPI *fpSetIOStatus)(int, bool);
	int   (WINAPI *fpGetIDCardName)(LPTSTR lpBuffer, int &nBufferLen);
	int   (WINAPI *fpGetDeviceSN)(LPTSTR lpDeviceID, int nSize);
	int   (WINAPI *fpGetTimeConsumed)(int nMainTimeType, int nSubTimeType);
	int   (WINAPI *fpGetSmartCardReaderVersion)(LPTSTR lpBuffer,int &nBufferLen);
	int   (WINAPI *fpGetAccessControlType)();
	bool InitDLL();
	void FreeDLL();
private:
	void *handler;
};
