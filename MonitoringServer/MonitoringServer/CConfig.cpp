#include "CConfig.h"

using namespace mylib;

CConfig* g_pConfig = CConfig::GetInstance();

// :NETWORK
//-----------------------------------
// 본 서버군 이름 / ServerLink 정보의 이름과 같아야 함
//-----------------------------------
WCHAR	CConfig::_szServerName[64];

//-----------------------------------
// 로그인서버 Listen IP/PORT
//
// 이는 서버에서 클라이언트 접속용 Listen 소켓 Bind
//-----------------------------------
WCHAR	CConfig::_szBindIP[16];
int		CConfig::_iBindPort;
WCHAR	CConfig::_szLanBindIP[16];
int		CConfig::_iLanBindPort;

//:SYSTEM
//-----------------------------------
// 모니터링서버 로그인 키
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