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

//-----------------------------------
// 모니터링서버 연결 IP/PORT
//-----------------------------------
WCHAR	CConfig::_szMonitoringLanIP[16];
int		CConfig::_iMonitoringLanPort;

//-----------------------------------
// IOCP 워커스레드 개수
//-----------------------------------
int		CConfig::_iWorkerThreadNo;


//:SYSTEM
//-----------------------------------
// 최대사용자
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