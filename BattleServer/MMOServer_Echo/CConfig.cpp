#include "CConfig.h"

using namespace mylib;

CConfig* g_pConfig = CConfig::GetInstance();

// :NETWORK
//-----------------------------------
// �� ������ �̸� / ServerLink ������ �̸��� ���ƾ� ��
//-----------------------------------
WCHAR	CConfig::_szServerName[64];

//-----------------------------------
// �α��μ��� Listen IP/PORT
//
// �̴� �������� Ŭ���̾�Ʈ ���ӿ� Listen ���� Bind
//-----------------------------------
WCHAR	CConfig::_szBindIP[16];
int		CConfig::_iBindPort;

//-----------------------------------
// ����͸����� ���� IP/PORT
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

//-----------------------------------
// DB
//-----------------------------------
WCHAR	CConfig::_szDBIP[16];
WCHAR	CConfig::_szDBAccount[64];
int		CConfig::_iDBPort;
WCHAR	CConfig::_szDBPassword[64];
WCHAR	CConfig::_szDBName[64];