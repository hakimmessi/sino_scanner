// TestLinux.cpp : Defines the entry point for the console application.
#include "IDCardRecog.h"
#include "ReadSID.h"
#include <unistd.h>
#include <wchar.h>
#include <sys/time.h>
using namespace std;

int main(int argc, char** argv)
{
    CReadSID readSID;
    CIDCardRecog idCardRecog;
	bool bRet = idCardRecog.InitDLL();
	if(!bRet)
		return 1;

	int nRet = idCardRecog.fpInitIDCard(L"UserID", 0, NULL); //填入userid

	if(nRet != 0)
	{
		cout << "InitIDCard failed, nRet = " << nRet << endl;
		idCardRecog.FreeDLL();
		return 2;
	}

	idCardRecog.fpSetConfigByFile(L"./IDCardConfig.ini");

	wchar_t pSN[20] = {0};
	idCardRecog.fpGetDeviceSN(pSN, 20);
	char pcSN[40] = {0};
	idCardRecog.fpWCharToUTF8Char(pcSN,pSN,40);
	cout << "SN: " << pcSN << endl;

	while(true)
	{
		cout<<endl;
		cout<<"********************Menu*********************"<<endl;
		cout<<"# 1 AutoRecog                               #"<<endl;
		cout<<"# 2 Recog                               	   #"<<endl;
		cout<<"# 3 exit                                    #"<<endl;
		cout<<"********************Menu*********************"<<endl;
		char cType;
		cin >> cType;

		if(cType == '1')
		{
			cout<<"1 AutoRecog"<<endl;
			int i = 0;
			while(true)
			{
				if(i == 0)
				{
					cout<<"Please insert a document "<<endl;
					i = 1;
				}

				if (readSID.ReadCard() == 0)
				{
					idCardRecog.fpBuzzerAlarm(200);
					cout << "读卡结束" << endl;
					usleep(50000);
					continue;
				}
				idCardRecog.fpSetIOStatus(5, false);
				idCardRecog.fpSetIOStatus(6, false);
				int nRet = idCardRecog.fpDetectDocument();
				if(nRet == 1)
				{
					i = 0;
					idCardRecog.fpSetIOStatus(5, true);
                    idCardRecog.fpSetIOStatus(6, false);
					cout<<"检测到证件放入，开始识别..."<<endl;
					int nCardType = 0;
					int nClassifyRet = -1;
					timeval startTime, endTime;
					gettimeofday(&startTime, 0);
					nClassifyRet = idCardRecog.fpAutoProcessIDCard(nCardType);
					gettimeofday(&endTime, 0);
					double dTimeuse = 1000000 * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_usec - startTime.tv_usec;
					cout << "AutoProcessIDCard Time: " << dTimeuse/1000 << "ms." << endl;
					cout << "Read RFID Time: " << idCardRecog.fpGetTimeConsumed(5, 0) << "ms.\n";
					idCardRecog.fpBuzzerAlarm(200);
					
					if (nClassifyRet > 0)
					{
                        int nIndex = 0;
						int nRet = 0;
                        TCHAR CardName[256] = {0};
                        int nNameLen = sizeof(TCHAR) * 256;
                        nRet = idCardRecog.fpGetIDCardName(CardName, nNameLen);
                        if (nRet == 0)
                        {
                            char tempName[512] = {0};
                            idCardRecog.fpWCharToUTF8Char(tempName, CardName, 512);
                            cout << "CardName：" << tempName << endl;
                        }
						cout << "*********************OCR Result*********************" << endl;
						while(true)
						{
							TCHAR buffer[512]={0};
							int nLenth = 512 * sizeof(TCHAR);
							char tempField[1024]={0};
							char tempResult[1024]={0};
							int nRet = idCardRecog.fpGetFieldNameEx(1, nIndex, buffer, nLenth);
							if(nRet==0)
							{
								idCardRecog.fpWCharToUTF8Char(tempField,buffer,1024);
							}
							else if(nRet==3)
							{
								break;
							}
							nLenth = 512 * sizeof(TCHAR);
							memset(buffer, 0, nLenth);
							idCardRecog.fpGetRecogResultEx(1, nIndex, buffer, nLenth);
							if (nLenth != 0)
							{
								cout << tempField << ": ";
								idCardRecog.fpWCharToUTF8Char(tempResult,buffer,1024);
								cout << tempResult << endl;
							}
							nIndex++;
						}
						
						if (nCardType == 1)
						{
							cout << "*********************RFID Result********************" << endl;
							nIndex = 0;
							while(true)
							{
								TCHAR buffer[512]={0};
								int nLenth = 512 * sizeof(TCHAR);
								char tempField[1024]={0};
								char tempResult[1024]={0};
								idCardRecog.fpGetFieldNameEx(0, nIndex, buffer, nLenth);
								idCardRecog.fpWCharToUTF8Char(tempField,buffer,1024);

								nLenth = 512 * sizeof(TCHAR);
								memset(buffer, 0, nLenth);
								int nRet = idCardRecog.fpGetRecogResultEx(0, nIndex, buffer, nLenth);
								if(nRet == 3)
									break;
								if (nLenth != 0)
								{
									cout << tempField << ": ";
									idCardRecog.fpWCharToUTF8Char(tempResult,buffer,1024);
									cout << tempResult << endl;
								}
								nIndex++;
							}
							if (idCardRecog.fpGetAccessControlType != NULL)
							{
								cout << "AccessType: " << idCardRecog.fpGetAccessControlType() << endl;
							}
						}
					}
					else
					{
						idCardRecog.fpSetIOStatus(5, false);
                        idCardRecog.fpSetIOStatus(6, true);
						cout << "AutoProcessIDCard failed, return " << nClassifyRet << endl;
					}
				}
				else
				{
					usleep(50000);
					continue;
				}
				usleep(50000);
				//break;
			}

		}
		else if (cType == '2')
		{
			int nCardType = 0;
			int nClassifyRet = -1;
			timeval startTime, endTime;
			gettimeofday(&startTime, 0);
			cout << "fpAutoProcessIDCard begin\n";
			nClassifyRet = idCardRecog.fpAutoProcessIDCard(nCardType);
			cout << "fpAutoProcessIDCard end\n";
			idCardRecog.fpSaveImageEx(L"./idcard.jpg", 3);
			gettimeofday(&endTime, 0);
			double dTimeuse = 1000000 * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_usec - startTime.tv_usec;
			cout << "识别结束, Time: " << dTimeuse/1000 << "ms." << endl;
			idCardRecog.fpBuzzerAlarm(200);
			
			if (nClassifyRet > 0)
			{
				int nIndex = 0;
				int nRet = 0;
				TCHAR CardName[256] = {0};
				int nNameLen = sizeof(TCHAR) * 256;
				nRet = idCardRecog.fpGetIDCardName(CardName, nNameLen);
				if (nRet == 0)
				{
					char tempName[512] = {0};
					idCardRecog.fpWCharToUTF8Char(tempName, CardName, 512);
					cout << "证件名称：" << tempName << endl;
				}
				cout << "*********************OCR Result*********************" << endl;
				while(true)
				{
					TCHAR buffer[512]={0};
					int nLenth = 512 * sizeof(TCHAR);
					char tempField[1024]={0};
					char tempResult[1024]={0};
					int nRet = idCardRecog.fpGetFieldNameEx(1, nIndex, buffer, nLenth);
					if(nRet==0)
					{
						idCardRecog.fpWCharToUTF8Char(tempField,buffer,1024);
					}
					else if(nRet==3)
					{
						break;
					}
					nLenth = 512 * sizeof(TCHAR);
					memset(buffer, 0, nLenth);
					idCardRecog.fpGetRecogResultEx(1, nIndex, buffer, nLenth);
					if (nLenth != 0)
					{
						cout << tempField << ": ";
						idCardRecog.fpWCharToUTF8Char(tempResult,buffer,1024);
						cout << tempResult << endl;
					}
					nIndex++;
				}
				
				if (nCardType == 1)
				{
					cout << "*********************RFID Result********************" << endl;
					nIndex = 0;
					while(true)
					{
						TCHAR buffer[512]={0};
						int nLenth = 512 * sizeof(TCHAR);
						char tempField[1024]={0};
						char tempResult[1024]={0};
						int nRet = idCardRecog.fpGetFieldNameEx(0, nIndex, buffer, nLenth);
						if(nRet==0)
						{
							idCardRecog.fpWCharToUTF8Char(tempField,buffer,1024);
						}
						else if(nRet==3)
						{
							break;
						}
						nLenth = 512 * sizeof(TCHAR);
						memset(buffer, 0, nLenth);
						idCardRecog.fpGetRecogResultEx(0, nIndex, buffer, nLenth);
						if (nLenth != 0)
						{
							cout << tempField << ": ";
							idCardRecog.fpWCharToUTF8Char(tempResult,buffer,1024);
							cout << tempResult << endl;
						}
						nIndex++;
					}
					if (idCardRecog.fpGetAccessControlType != NULL)
					{
						cout << "AccessType: " << idCardRecog.fpGetAccessControlType() << endl;
					}
				}
			}
			else
			{
				idCardRecog.fpSetIOStatus(5, false);
				idCardRecog.fpSetIOStatus(6, true);
				cout << "AutoProcessIDCard failed, return " << nClassifyRet << endl;
			}
			cout << "----------Recog end.----------" << endl;
		}
		else if(cType == '3')
		{
			break;
		}
		else
		{
			cout << "---------" << endl;
		}
	}
	
	cout << "end" << endl;
	return 0;
}
