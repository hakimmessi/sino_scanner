#include "ReadSID.h"
using namespace std;

CReadSID::CReadSID()
{
    m_handle = NULL;
    m_bSID = false;
    m_nPort = 0;
    m_bLoad = false;
    memset(m_cCardType, 0, 2);
    LoadLib();
}

CReadSID::~CReadSID()
{
	if (m_nPort != 0)
	{
		SDT_ClosePort(m_nPort);
		m_nPort = 0;
	}
	if (m_handle)
    {
		dlclose(m_handle);
		m_handle = NULL;
    }
    m_bLoad = false;
}

string CReadSID::Utf16To8(char *source)
{
    char buf8[256]= {0};
	Utf16_To_Utf8((const UTF16*)source,(UTF8*)buf8,256,strictConversion);
	return string(buf8);
}

int CReadSID::Utf16_To_Utf8(const UTF16* sourceStart, UTF8* targetStart, size_t outLen, ConversionFlags flags)
{
	int result = 0;
	const UTF16* source = sourceStart;
	UTF8* target = targetStart;
	UTF8* targetEnd = targetStart + outLen;

	if ((NULL == source) || (NULL == targetStart)) {
		printf("ERR, Utf16_To_Utf8: source=%p, targetStart=%p\n", source, targetStart);
		return conversionFailed;
	}

	while (*source) {
		UTF32 ch;
		unsigned short bytesToWrite = 0;
		const UTF32 byteMask = 0xBF;
		const UTF32 byteMark = 0x80;
		const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */
		ch = *source++;
		/* If we have a surrogate pair, convert to UTF32 first. */
		if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
			/* If the 16 bits following the high surrogate are in the source buffer... */
			if (*source) {
				UTF32 ch2 = *source;
				/* If it's a low surrogate, convert to UTF32. */
				if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
					ch = ((ch - UNI_SUR_HIGH_START) << halfShift) + (ch2 - UNI_SUR_LOW_START) + halfBase;
					++source;
				}
				else if (flags == strictConversion) { /* it's an unpaired high surrogate */
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
			}
			else { /* We don't have the 16 bits following the high surrogate. */
				--source; /* return to the high surrogate */
				result = sourceExhausted;
				break;
			}
		}
		else if (flags == strictConversion) {
			/* UTF-16 surrogate values are illegal in UTF-32 */
			if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
				--source; /* return to the illegal value itself */
				result = sourceIllegal;
				break;
			}
		}
		/* Figure out how many bytes the result will require */
		if (ch < (UTF32)0x80) {
			bytesToWrite = 1;
		}
		else if (ch < (UTF32)0x800) {
			bytesToWrite = 2;
		}
		else if (ch < (UTF32)0x10000) {
			bytesToWrite = 3;
		}
		else if (ch < (UTF32)0x110000) {
			bytesToWrite = 4;
		}
		else {
			bytesToWrite = 3;
			ch = UNI_REPLACEMENT_CHAR;
		}

		target += bytesToWrite;
		if (target > targetEnd) {
			source = oldSource; /* Back up source pointer! */
			target -= bytesToWrite; result = targetExhausted; break;
		}
		switch (bytesToWrite) { /* note: everything falls through. */
		case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
		case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
		case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
		case 1: *--target = (UTF8)(ch | firstByteMark[bytesToWrite]);
		}
		target += bytesToWrite;
	}
	return result;
}

int CReadSID::LoadLib()
{
#ifdef ANDROID_PLAT
    m_handle = dlopen("libzysdtapi.so", RTLD_NOW);
#else
	m_handle = dlopen("./libzysdtapi.so", RTLD_NOW);
#endif
	if (m_handle == NULL)
	{
		printf("dlopen failed\r\n");
		printf("%s\n",dlerror());
		return 1;
	}
	SDT_OpenPort = (int (__stdcall *)(int))dlsym(m_handle, "SDT_OpenPort");
	SDT_ClosePort = (int (__stdcall *)(int))dlsym(m_handle, "SDT_ClosePort");
	SDT_StartFindIDCard = (int (__stdcall *)(int, unsigned char*, int))dlsym(m_handle, "SDT_StartFindIDCard");
	SDT_SelectIDCard = (int (__stdcall *)(int, unsigned char*, int))dlsym(m_handle, "SDT_SelectIDCard");
	SDT_ReadBaseMsg = (int (__stdcall *)(int, unsigned char*, unsigned int*, unsigned char*, unsigned int*, int))dlsym(m_handle, "SDT_ReadBaseMsg");
	SDT_ReadNewAppMsg = (int (__stdcall *)(int, unsigned char*, unsigned int*, int))dlsym(m_handle, "SDT_ReadNewAppMsg");
	SDT_GetSAMIDToStr = (int (__stdcall *)(int, char*, int))dlsym(m_handle, "SDT_GetSAMIDToStr");
	if (SDT_OpenPort == NULL || SDT_ClosePort == NULL || SDT_StartFindIDCard == NULL ||
		SDT_SelectIDCard == NULL || SDT_ReadBaseMsg == NULL || SDT_ReadNewAppMsg == NULL ||
		SDT_GetSAMIDToStr == NULL)
	{
		dlclose(m_handle);
		m_handle = NULL;
		printf("export sdtapi func failed.\n");
		return 2;
	}
	handle_100ud = dlopen("./lib100UD.so", RTLD_NOW);
	if (handle_100ud == NULL)
    {
        printf("load 100ud.so failed");
        return 3;
    }
	DecodingPhotos = (int (__stdcall *)(char *, int , char *, int ))dlsym(handle_100ud,"DecodingPhotos");
	if(DecodingPhotos==NULL)
	{
		dlclose(handle_100ud);
		handle_100ud = NULL;
		printf("export sdtapi func failed.\n");
		return 4;
	}
	int nRet = 0;
	for (int iPort = 1001; iPort < 1017; ++iPort)
	{
		nRet = SDT_OpenPort(iPort);
		if (nRet == 0x90)
		{
			m_nPort = iPort;//打开端口成功
			break;
		}
	}
	if(nRet != 0x90)
	{
		dlclose(m_handle);
		m_handle = NULL;
		printf("Open Port failed, ret = %d\n", nRet);
		return 5;
	}
	m_bLoad = true;
	return 0;
}

string CReadSID::GetName()
{
	if (0 == m_nMsgLen)
		return "";
	char szName[128] = {0};
	if(m_bSID || m_bGAJ)
		strncpy(szName, (char*)m_pucCHMsg, 15);
	else
		strncpy(szName, (char*)m_pucCHMsg, 60);
	return string("姓名：") + Utf16To8(szName);
}

string CReadSID::GetPeopleChineseName()
{
    char szName[40] = {0};//中文姓名占30字节，第5号，前有4项数据项共占158字节
	strncpy(szName, (char*)(m_pucCHMsg+158), 30);// m_pucCHMsg[1280]文字信息
	return string("中文姓名：") + Utf16To8(szName);
}

string CReadSID::GetCardVersion()
{
	char szVersion[30] = {0};//证件版本号占4字节，第9号，前有8项数据项共占236字节
	strncpy(szVersion, (char*)(m_pucCHMsg + 236), 4);
	return string("证件版本号：") + Utf16To8(szVersion);
}

string CReadSID::GetReverse()
{
	char szTmp[30] = {0};
	strncpy(szTmp, (char*)(m_pucCHMsg + 250), 6);
	return string("预留项：") + Utf16To8(szTmp);
}

string CReadSID::GetSex()
{
	char szTmp[10] = {0};
	if(m_bSID || m_bGAJ)
	{
        strncpy(szTmp, (char*)(m_pucCHMsg + 30), 2);
	}
	else
	{
		strncpy(szTmp, (char*)(m_pucCHMsg + 120), 2);
    }
	int nRet=atoi(szTmp);
	if (nRet==1)
		return string("性别：男");
	else
		return string("性别：女");
}

string CReadSID::GetPeople()
{
	char szTmp[10] = {0};
	int i=0;
	if(m_bSID)
	{
		memcpy(szTmp, (char*)(m_pucCHMsg + 32), 4);
	}
	else
	{
		memcpy(szTmp, (char*)(m_pucCHMsg + 152), 6);
	}
	char buf8[20]={0};
	Utf16_To_Utf8((const UTF16*)szTmp,(UTF8*)buf8,10,strictConversion);
	int nRet=atoi(buf8);//民族划分
    switch(nRet)
    {
    case 01:  return string("民族：") + string("汉");break;
    case 02:  return string("民族：") + string("蒙古");break;
    case 03:  return string("民族：") + string("回");break;
    case 04:  return string("民族：") + string("藏");break;
    case 05:  return string("民族：") + string("维吾尔");break;
    case 06:  return string("民族：") + string("苗");break;
    case 07:  return string("民族：") + string("彝");break;
    case 8:   return string("民族：") + string("壮");break;
    case 9:   return string("民族：") + string("布依");break;
    case 10:  return string("民族：") + string("朝鲜");break;
    case 11:  return string("民族：") + string("满");break;
    case 12:  return string("民族：") + string("侗");break;
    case 13:  return string("民族：") + string("瑶");break;
    case 14:  return string("民族：") + string("白");break;
    case 15:  return string("民族：") + string("土家");break;
    case 16:  return string("民族：") + string("哈尼");break;
    case 17:  return string("民族：") + string("哈萨克");break;
    case 18:  return string("民族：") + string("傣");break;
    case 19:  return string("民族：") + string("黎");break;
    case 20:  return string("民族：") + string("傈僳");break;
    case 21:  return string("民族：") + string("佤");break;
    case 22:  return string("民族：") + string("畲");break;
    case 23:  return string("民族：") + string("高山");break;
    case 24:  return string("民族：") + string("拉祜族");break;
    case 25:  return string("民族：") + string("水");break;
    case 26:  return string("民族：") + string("东乡");break;
    case 27:  return string("民族：") + string("纳西");break;
    case 28:  return string("民族：") + string("景颇");break;
    case 29:  return string("民族：") + string("柯尔克孜");break;
    case 30:  return string("民族：") + string("土");break;
    case 31:  return string("民族：") + string("达斡尔");break;
    case 32:  return string("民族：") + string("仫佬");break;
    case 33:  return string("民族：") + string("羌");break;
    case 34:  return string("民族：") + string("布朗");break;
    case 35:  return string("民族：") + string("撒拉");break;
    case 36:  return string("民族：") + string("毛南");break;
    case 37:  return string("民族：") + string("仡佬");break;
    case 38:  return string("民族：") + string("锡伯");break;
    case 39:  return string("民族：") + string("阿昌");break;
    case 40:  return string("民族：") + string("普米");break;
    case 41:  return string("民族：") + string("塔吉克");break;
    case 42:  return string("民族：") + string("怒");break;
    case 43:  return string("民族：") + string("乌兹别克");break;
    case 44:  return string("民族：") + string("俄罗斯");break;
    case 45:  return string("民族：") + string("鄂温克");break;
    case 46:  return string("民族：") + string("德昂");break;
    case 47:  return string("民族：") + string("保安");break;
    case 48:  return string("民族：") + string("裕固");break;
    case 49:  return string("民族：") + string("京");break;
    case 50:  return string("民族：") + string("塔塔尔");break;
    case 51:  return string("民族：") + string("独龙");break;
    case 52:  return string("民族：") + string("鄂伦春");break;
    case 53:  return string("民族：") + string("赫哲");break;
    case 54:  return string("民族：") + string("门巴");break;
    case 55:  return string("民族：") + string("珞巴");break;
    case 56:  return string("民族：") + string("基诺");break;
    case 97:  return string("民族：") + string("其他");break;
    case 98:  return string("民族：") + string("外国血统中国籍人士");break;
    default : return string("民族：") + string("");break;
    }
}

string CReadSID::GetBirthday()
{
	char szTmp[30] = {0};
	if(m_bSID || m_bGAJ)
		memcpy(szTmp, (char*)(m_pucCHMsg + 36), 16);
	else
		memcpy(szTmp, (char*)(m_pucCHMsg + 220), 16);
	return string("出生日期：") + Utf16To8(szTmp);
}

string CReadSID::GetAddress()
{
	char szTmp[100] = {0};
	memcpy(szTmp, (char*)(m_pucCHMsg + 52), 70);
	return string("住址：") + Utf16To8(szTmp);
}

string CReadSID::GetAuthority()
{
	char szTmp[50] = {0};
	if(m_bSID || m_bGAJ)
	{
        memcpy(szTmp, (char*)(m_pucCHMsg + 158), 30);
        return string("签发机关：") + Utf16To8(szTmp);
	}
	else
	{
		memcpy(szTmp, (char*)(m_pucCHMsg + 240), 8);
		return string("当次申请受理机关：") + Utf16To8(szTmp);
	}
}

string CReadSID::GetIDNumber()
{
	char szTmp[40] = {0};
	if(m_bSID || m_bGAJ)
	{
		memcpy(szTmp, (char*)(m_pucCHMsg + 122), 36);
		return string("身份证号：") + Utf16To8(szTmp);
	}
	else
	{
		memcpy(szTmp, (char*)(m_pucCHMsg + 122), 30);
		return string("永久居留证号码：") + Utf16To8(szTmp);
	}
}

string CReadSID::GetIssueDay()
{
	char szTmp[30] = {0};
	memcpy(szTmp, (char*)(m_pucCHMsg + 188), 16);
	if(m_bSID || 0 == strcmp((char*)m_cCardType, "J"))
	{
		return string("有效日期起始日期：") + Utf16To8(szTmp);
	}
	else
	{
		return string("证件签发日期：") + Utf16To8(szTmp);
	}
}

string CReadSID::GetExpityDay()
{
	char szTmp[30] = {0};
	memcpy(szTmp, (char*)(m_pucCHMsg + 204), 16);
	if(m_bSID || m_bGAJ)
	{
		return string("有效日期截止日期：") + Utf16To8(szTmp);

	}
	else
	{
		return string("证件终止日期：") + Utf16To8(szTmp);
	}
}

string CReadSID::GetPassNum()
{
	char szTmp[30] = {0};
	memcpy(szTmp, (char*)(m_pucCHMsg + 220), 18);
	return string("通行证号码：") + Utf16To8(szTmp);
}

string CReadSID::GetIssueNum()
{
	char szTmp[10] = {0};
	memcpy(szTmp, (char*)(m_pucCHMsg + 238), 4);
	return string("签发次数：") + Utf16To8(szTmp);
}

void CReadSID::ShowCHSID()
{
	if (m_bSID)
	{
        cout << GetName() << endl;
        cout << GetSex() << endl;
        cout << GetIDNumber() << endl;
        cout << GetPeople() << endl;
        cout << GetIssueDay() << endl;
        cout << GetExpityDay() << endl;
        cout << GetBirthday() << endl;
        cout << GetAuthority() << endl;
        cout << GetAddress() << endl;
	}
	else if (m_bForeigner)
	{
        cout << GetName() << endl;
        cout << GetSex() << endl;
        cout << GetIDNumber() << endl;
        cout << GetPeople() << endl;
        cout << GetPeopleChineseName() << endl;
        cout << GetIssueDay() << endl;
        cout << GetExpityDay() << endl;
        cout << GetBirthday() << endl;
        cout << GetCardVersion() << endl;
        cout << GetAuthority() << endl;
        cout << GetReverse() << endl;
	}
	else if (m_bGAJ)
	{
        cout << GetName() << endl;
        cout << GetSex() << endl;
        cout << GetBirthday() << endl;
        cout << GetAddress() << endl;
        cout << GetIDNumber() << endl;
        cout << GetAuthority() << endl;
        cout << GetIssueDay() << endl;
        cout << GetExpityDay() << endl;
        cout << GetPassNum() << endl;
        cout << GetIssueNum() << endl;
	}
	else
	{
	}
}

int CReadSID::ReadCard()
{
    if (!m_bLoad)
    {
        return -1;
    }
	unsigned int len = 0;
	unsigned char pucApp[320] = {0};
	int nRet = SDT_ReadNewAppMsg(m_nPort, pucApp, &len, 0);
	if(0x90 == nRet || 0x91 == nRet)
	{
		return 1;
	}

	unsigned char pucIIN[4] = {0};
	nRet = SDT_StartFindIDCard(m_nPort, pucIIN, 0);
	if(nRet != 0x9F)
	{
		return 2;
	}

	unsigned char pucSN[8] = {0};
	nRet = SDT_SelectIDCard(m_nPort, pucSN, 0);
	if(nRet != 0x90)
	{
		return 3;
	}

	memset(m_pucCHMsg, 0, 512);
	memset(m_pucPHMsg, 0, 1024);
	m_nMsgLen = 0;
	m_nPHLen = 0;
	nRet = SDT_ReadBaseMsg(m_nPort, m_pucCHMsg, &m_nMsgLen, m_pucPHMsg, &m_nPHLen, 0);
	if (nRet != 0x90)
	{
		return 4;
	}
	if (m_nMsgLen == 0)
	{
		return 5;
	}
	else
	{
		if (m_nPHLen > 0)
		{
			// fstream outfile("./SIDHead.wlt", ios::binary | ios::out | ios::ate);
			// if (outfile.is_open())
			// {
			// 	outfile.write((char*)m_pucPHMsg, 1024);
			// 	outfile.close();
			// }
			cout<<"m_nPHLen= "<< m_nPHLen <<endl;
			unsigned char WltData[1024];
			int WltLen = 1024;
			unsigned char arrPhotoData[38862];
			int iPhotoDataLen = sizeof(arrPhotoData);
			int iRet = DecodingPhotos((char*)m_pucPHMsg,m_nPHLen,(char*)arrPhotoData, iPhotoDataLen);
			if(iRet != 38862)
			{
				printf("DecodingPhotos failed, Ret = %d\n", iRet);
				//return -1;
			}
			
			FILE* fp_photo = fopen("./TestChipSIDHead.bmp","wb+");
			if(fp_photo == NULL)
			{
				printf(" write  Photo error\n");
				//return -1;
			}
			fwrite(arrPhotoData, 1, iPhotoDataLen, fp_photo);
			fclose(fp_photo);
			//return 0;

		}
		memset(m_cCardType, 0, 2);
		strncpy((char*)m_cCardType, (char*)(m_pucCHMsg + 248), 2);
		if(0 == strcmp((char*)m_cCardType, "I"))				// 大写I表示外国人永久居留证
		{
			cout << "外国人永久居留证" << endl;
			m_bSID = false;
			m_bGAJ = false;
			m_bForeigner = true;
			ShowCHSID();
		}
		else if(0 == strcmp((char*)m_cCardType, "J"))        // 大写J表示港澳台居民居住证
		{
			cout << "港澳台居民居住证" << endl;
			m_bSID = false;
			m_bGAJ = true;
			m_bForeigner = false;
			ShowCHSID();
		}
		else
		{
			cout << "二代身份证" << endl;
			m_bSID = true;
			m_bGAJ = false;
			m_bForeigner = false;
			ShowCHSID();
		}
		return 0;
	}
}
