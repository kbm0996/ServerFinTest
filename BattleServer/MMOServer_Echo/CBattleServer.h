#ifndef __NETWORK_H__
#define __NETWORK_H__
#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CMMOServer.h"
#include "CommonProtocol.h"
#include "CMonitorClient.h"
#include "CCpuUsage.h"
#include "CConfig.h"
#include <Pdh.h>
#pragma comment(lib, "pdh.lib")
#include <list>

using namespace std;
using namespace mylib;


class CPlayer;
class CBattleServer : public CMMOServer
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Server Control
	//
	//////////////////////////////////////////////////////////////////////////
	CBattleServer(int iMaxSession);
	virtual ~CBattleServer();
	
	// Server ON/OFF
	bool	Start();
	bool	Stop();

	// 1000ms마다 호출
	void	Monitoring();

private:
	// AUTH 모드 업데이트 이벤트 로직처리부
	virtual	void	OnAuth_Process(void);

	// GAME 모드 업데이트 이벤트 로직처리부
	virtual	void	OnGame_Process(void);

	bool InitPDH();	// 생성자에서 호출

private:
	bool			_bShutdown;

	CPlayer*		_pPlayer;
	CMonitorClient* _pMonitorClient;

	// CPU Usage
	CCpuUsage		_CPUTime;
	// PDH
	PDH_HQUERY		_PDHQuery;
	PDH_HCOUNTER	_pdh_Counter_PrivateBytes;
};

class CPlayer : public CNetSession
{
public:
	CPlayer();
	virtual ~CPlayer();

private:
	//////////////////////////////////////////////////////////////////////////
	// Handler
	//  Auth 접속 - AuthPacket 송수신 - Auth 종료 - Game 접속 - GamePacket 송수신 - Game 종료 - Release 
	//
	// OnAuth_ClientJoin	: 처음 접속. 인증 데이터 초기화
	// OnAuth_ClientLeave	: 인증 종료. 인증 데이터 정리
	// OnAuth_Packet		: 로그인 패킷 처리.
	// OnGame_ClientJoin	: 게임 접속. 게임 데이터 초기화
	// OnGame_ClientLeave	: 게임 종료. 게임 데이터 정리
	// OnGame_Packet		: 게임 패킷 처리.
	// OnGame_ClientRelease	: 최종 접속 종료
	//////////////////////////////////////////////////////////////////////////
	virtual void	OnAuth_ClientJoin(void);
	virtual void	OnAuth_ClientLeave(void);
	virtual void	OnAuth_Packet(CNPacket* pPacket);
	virtual void	OnGame_ClientJoin(void);
	virtual void	OnGame_ClientLeave(void);
	virtual void	OnGame_Packet(CNPacket* pPacket);
	virtual void	OnGame_ClientRelease(void);
	virtual void	OnTimeout(void);


	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	void OnAuth_PacketProc_REQ_LOGIN(CNPacket* pPacket);

	void OnGame_PacketProc_REQ_ECHO(CNPacket* pPacket);
	void OnGame_PacketProc_REQ_HEARTBEAT();

	void mpResLogin(CNPacket * pBuffer, BYTE byStatus, INT64 iAccountNo);


	friend class CBattleServer;
	CBattleServer*	_pServer;

	INT64	_iAccountNo;
	char	_szSessionKey[64];
};

#endif