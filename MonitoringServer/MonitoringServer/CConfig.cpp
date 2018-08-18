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
WCHAR	CConfig::_szLanBindIP[16];
int		CConfig::_iLanBindPort;

//:SYSTEM
//-----------------------------------
// ����͸����� �α��� Ű
//-----------------------------------
char	CConfig::_szLoginSessionKey[33];

//-----------------------------------
// Packet Encode Key
//-----------------------------------
int		CConfig::_byPacketCode;
int		CConfig::_byPacketKey1;
int		CConfig::_byPacketKey2;

int		CConfig::_AutoConnect;

//-----------------------------------
// DB
//-----------------------------------
WCHAR	CConfig::_szDBIP[16];
WCHAR	CConfig::_szDBAccount[64];
int		CConfig::_iDBPort;
WCHAR	CConfig::_szDBPassword[64];
WCHAR	CConfig::_szDBName[64];