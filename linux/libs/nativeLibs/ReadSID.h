#pragma once
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <string>
#include <string.h>
#include <locale.h>
#include "utf.h"
using namespace std;
#define __stdcall

typedef int ( __stdcall *SDT_OpenPortDef )( int iPort );
typedef int ( __stdcall *SDT_ClosePortDef )( int iPort );
typedef int ( __stdcall *SDT_StartFindIDCardDef )( int iPort, unsigned char *pRAPDU, int iIfOpen );
typedef int ( __stdcall *SDT_SelectIDCardDef )( int iPort, unsigned char *pRAPDU, int iIfOpen );
typedef int ( __stdcall *SDT_ReadBaseMsgToFileDef )( int iPortID, char *pcCHMsgFileName, unsigned int *puiCHMsgFileLen, char *pcPHMsgFileName, unsigned int *puiPHMsgFileLen, int iIfOpen );
typedef int ( __stdcall *SDT_ReadBaseMsgDef )( int iPort, unsigned char *pucCHMsg, unsigned int *puiCHMsgLen, unsigned char *pucPHMsg, unsigned int *puiPHMsgLen, int iIfOpen );
typedef int ( __stdcall *SDT_ReadBaseFPMsgDef )( int iPort, unsigned char *pucCHMsg, unsigned int *puiCHMsgLen, unsigned char *pucPHMsg, unsigned int *puiPHMsgLen, unsigned char *pucFPMsg, unsigned int *puiFPMsgLen, int iIfOpen );
typedef int ( __stdcall *SDT_ReadBaseFPMsgToFileDef )( int iPortID, char *pcCHMsgFileName, unsigned int *puiCHMsgFileLen, char *pcPHMsgFileName, unsigned int *puiPHMsgFileLen, char *pcFPMsgFileName, unsigned int *puiFPMsgFileLen, int iIfOpen );
typedef int ( __stdcall *SDT_ReadNewAppMsgDef )( int iPort, unsigned char *pucAppMsg, unsigned int *puiAppMsgLen, int iIfOpen );
typedef int ( __stdcall *SDT_GetSAMIDDef )( int iPort, unsigned char *pucSAMID, int iIfOpen );
typedef int ( __stdcall *SDT_GetSAMIDToStrDef )( int iPort, char *pcSAMID, int iIfOpen );

typedef int ( __stdcall *p_DecodingPhotos)(char *inBuf, int inBufsize, char *OutBuf, int outBufsize);

class CReadSID
{
public:
    CReadSID();
    ~CReadSID();
    int Utf16_To_Utf8(const UTF16* sourceStart, UTF8* targetStart, size_t outLen, ConversionFlags flags);
    string Utf16To8(char *source);
    int LoadLib();
    int ReadCard();
    void ShowCHSID();
    string GetName();
    string GetPeopleChineseName();// 获取外国人中文姓名
    string GetCardVersion(); // 获得证件版本号
    string GetReverse();// 获取预留项
    string GetSex();
    string GetPeople();//民族
    string GetBirthday();
    string GetAddress();
    string GetAuthority();
    string GetIDNumber();
    string GetIssueDay();//起始日期
    string GetExpityDay();//截止日期
    string GetPassNum(); //通行证号码
    string GetIssueNum();//签发次数

private:
    SDT_OpenPortDef             SDT_OpenPort;
    SDT_ClosePortDef            SDT_ClosePort;
    SDT_StartFindIDCardDef      SDT_StartFindIDCard;
    SDT_SelectIDCardDef         SDT_SelectIDCard;
    SDT_ReadBaseMsgDef          SDT_ReadBaseMsg;
    SDT_ReadBaseFPMsgDef        SDT_ReadBaseFPMsg;
    SDT_ReadBaseMsgToFileDef    SDT_ReadBaseMsgToFile;
    SDT_ReadBaseFPMsgToFileDef  SDT_ReadBaseFPMsgToFile;
    SDT_ReadNewAppMsgDef        SDT_ReadNewAppMsg;
    SDT_GetSAMIDDef             SDT_GetSAMID;
    SDT_GetSAMIDToStrDef        SDT_GetSAMIDToStr;
    p_DecodingPhotos DecodingPhotos = NULL;
    void *m_handle;
    void *handle_100ud;
    int m_nPort;
    bool m_bLoad;
    bool m_bSID;
    bool m_bGAJ;
    bool m_bForeigner;
    unsigned char m_cCardType[2];
    unsigned char m_pucCHMsg[512]; //文字信息
	unsigned char m_pucPHMsg[1024]; //照片信息
	unsigned int m_nMsgLen;
	unsigned int m_nPHLen;
};
