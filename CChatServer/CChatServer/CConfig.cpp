#include "CConfig.h"

using namespace mylib;

CConfig* g_pConfig = CConfig::GetInstance();

// :NETWORK
//-----------------------------------
// �� ������ �̸� / ServerLink ������ �̸��� ���ƾ� ��
//-----------------------------------
WCHAR	CConfig::_szServerName[20];

//-----------------------------------
// ä�ü��� Listen IP/PORT
//
// �̴� �������� Ŭ���̾�Ʈ ���ӿ� Listen ���� Bind
//-----------------------------------
WCHAR	CConfig::_szBindIP[16];
int		CConfig::_iBindPort;

//-----------------------------------
// �α��μ��� ���� IP/PORT
//-----------------------------------
WCHAR	CConfig::_szLoginServerIP[16];
int		CConfig::_iLoginServerPort;
WCHAR	CConfig::_szLoginLanServerIP[16];
int		CConfig::_iLoginLanServerPort;

//-----------------------------------
// ����͸� ���� ���� IP/PORT
//-----------------------------------
WCHAR	CConfig::_szMonitoringLanIP[16];
int		CConfig::_iMonitoringLanPort;

//-----------------------------------
// IOCP ��Ŀ������ ����
//-----------------------------------
int		CConfig::_iWorkerThreadNo;


//:SYSTEM
//-----------------------------------
// �ִ�����
//-----------------------------------
int		CConfig::_iClientMax;

//-----------------------------------
// Packet Encode Key
//-----------------------------------
int		CConfig::_byPacketCode;
int		CConfig::_byPacketKey1;
int		CConfig::_byPacketKey2;