#include "IDCardRecog.h"
#include <iostream>
#include <stdio.h>
#include <dlfcn.h>
using namespace std;
CIDCardRecog::CIDCardRecog(void)
{

}

CIDCardRecog::~CIDCardRecog(void)
{
	FreeDLL();
}
void CIDCardRecog::FreeDLL()
{
	if(fpFreeIDCard)
	{
		fpFreeIDCard();
	}
	if(handler)
	{
		dlclose(handler);
		handler=NULL;
	}
	fpInitIDCard = NULL;
	fpFreeIDCard = NULL;
	fpSetIDCardID = NULL;
	fpAddIDCardID = NULL;
	fpGetSubID = NULL;
	fpGetVersionInfo = NULL;
	fpDetectDocument = NULL;
	fpSaveImageEx = NULL;
	fpGetDataGroupContent = NULL;
	fpAutoProcessIDCard = NULL;
	fpSetConfigByFile = NULL;
	fpResetIDCardID = NULL;
	fpUTF8CharToWChar = NULL;
	fpWCharToUTF8Char = NULL;
	fpBuzzerAlarm = NULL;
	fpSetIOStatus = NULL;
	fpGetIDCardName = NULL;
	fpGetDeviceSN = NULL;
	fpGetTimeConsumed = NULL;
	fpGetAccessControlType = NULL;
	fpGetSmartCardReaderVersion = NULL;
}
bool CIDCardRecog::InitDLL()
{
	handler=NULL;
#ifdef ANDROID_PLAT
	handler = dlopen("libIDCard.so", RTLD_NOW);
#else
	handler = dlopen("./libIDCard.so", RTLD_NOW);
#endif
	if(handler==NULL)
	{
		printf("dlopen failed\r\n");
		printf("%s\n",dlerror());
		return false;
	}
	fpInitIDCard = (int (*)(LPCTSTR, int, LPCTSTR))dlsym(handler,"InitIDCard");
	fpFreeIDCard=(void (*)())dlsym(handler,"FreeIDCard");
	fpSetIDCardID=(int(*)(int nMainID,int nSubID[],int nSubIdCount))dlsym(handler,"SetIDCardID");
	fpAddIDCardID=(int(*)(int nMainID,int nSubID[],int nSubIdCount))dlsym(handler,"AddIDCardID");
	fpGetSubID=(int(*)())dlsym(handler,"GetSubID");
	fpGetVersionInfo=(void(*)(LPCTSTR ,int))dlsym(handler,"GetVersionInfo");
	fpSaveImageEx=(int(* )(LPCTSTR lpFileName,int nType))dlsym(handler,"SaveImageEx");
	fpGetDataGroupContent=(int(*)(int nDGIndex,bool bRawData,unsigned char * lpBuffer,int &len))dlsym(handler,"GetDataGroupContent");
	fpGetRecogResultEx = (int (*)(int nAttribute,int nIndex,LPTSTR lpBuffer,int &nBufferLen))dlsym(handler,"GetRecogResultEx");
	fpGetFieldNameEx = (int (*)(int nAttribute,int nIndex,LPTSTR lpBuffer,int &nBufferLen))dlsym(handler,"GetFieldNameEx");
	fpGetFieldConfEx = (int (*)(int nAttribute,int nIndex))dlsym(handler,"GetFieldConfEx");
	fpGetResultTypeEx = (int (*)(int nAttribute,int nIndex))dlsym(handler,"GetResultTypeEx");
	fpAutoProcessIDCard = (int(*)(int &))dlsym(handler, "AutoProcessIDCard");
	fpSetConfigByFile = (int(*)(LPCWSTR))dlsym(handler, "SetConfigByFile");
	fpDetectDocument = (int (*)())dlsym(handler, "DetectDocument");
	fpResetIDCardID = (void (*)())dlsym(handler, "ResetIDCardID");
	fpUTF8CharToWChar=(int(*)(wchar_t* pwszDest, const char* pszSrc, int nWcharLen))dlsym(handler,"UTF8CharToWChar");
	fpWCharToUTF8Char=(int(*)(char* pszDest,const wchar_t* pwszSrc,int nCharLen))dlsym(handler,"WCharToUTF8Char");
	fpBuzzerAlarm = (int(*)(int))dlsym(handler, "BuzzerAlarm");
	fpSetIOStatus = (int(*)(int, bool))dlsym(handler, "SetIOStatus");
	fpGetIDCardName = (int(*)(LPTSTR, int&))dlsym(handler, "GetIDCardName");
	fpGetDeviceSN = (int(*)(LPTSTR, int))dlsym(handler, "GetDeviceSN");
	fpGetTimeConsumed = (int(*)(int, int))dlsym(handler, "GetTimeConsumed");
	fpGetAccessControlType = (int(*)())dlsym(handler, "GetAccessControlType");
	fpGetSmartCardReaderVersion = (int(*)(LPTSTR, int&))dlsym(handler, "GetSmartCardReaderVersion");
	if( fpInitIDCard == NULL
		|| fpFreeIDCard == NULL
		|| fpGetVersionInfo == NULL
		|| fpGetSubID == NULL
		|| fpSetIDCardID == NULL
		|| fpAddIDCardID == NULL
		|| fpSaveImageEx==NULL
		|| fpGetDataGroupContent==NULL
		|| fpGetRecogResultEx==NULL
		|| fpGetFieldNameEx==NULL
		|| fpGetFieldConfEx==NULL
		|| fpGetResultTypeEx==NULL
		|| fpAutoProcessIDCard == NULL
		|| fpSetConfigByFile == NULL
		|| fpResetIDCardID == NULL
		|| fpDetectDocument == NULL
		|| fpUTF8CharToWChar == NULL
		|| fpWCharToUTF8Char == NULL
		|| fpBuzzerAlarm == NULL
		|| fpSetIOStatus == NULL
		|| fpGetIDCardName == NULL
		|| fpGetDeviceSN == NULL
		|| fpGetTimeConsumed == NULL
		)
	{
		printf("export dll failed\r\n");
		dlclose(handler);
		handler = NULL;
		return false;
	}
	return true;
}
