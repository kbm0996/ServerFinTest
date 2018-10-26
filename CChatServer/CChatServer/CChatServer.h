#ifndef __NETWORK_H__
#define __NETWORK_H__
#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CNetServer.h"
#include "CMonitorClient.h"
#include "CLoginClient.h"
#include "CConfig.h"
#include "CommonProtocol.h"
#include "CCpuUsage.h"
#include <Pdh.h>
#pragma comment(lib, "pdh.lib")
#include <list>
#include <map>

using namespace std;
using namespace mylib;

class CChatServer : public CNetServer
{
public:
	friend class CLoginClient;
	enum en_SERVER_CONFIG
	{
		// Message Type
		en_MSG_JOIN = 0,
		en_MSG_LEAVE,
		en_MSG_PACKET,
		en_MSG_HEARTBEAT,

		// Sector Cnt
		en_SECTOR_MAX_Y = 100,
		en_SECTOR_MAX_X = 100,

		// Heartbeat timeout
		en_HEART_BEAT_INTERVAL = 30000
	};

#pragma pack(push, 1)
	struct st_MESSAGE
	{
		WORD		wType;
		UINT64		iSessionID;
		CNPacket*	pPacket;
	};
#pragma pack(pop)
	struct st_PLAYER
	{
		UINT64	iSessionID;
		INT64	iAccountNo;
		WCHAR	szID[20];
		WCHAR	szNickname[20];
		char	szSessionKey[64];

		short	shSectorX, shSectorY;
		ULONGLONG lLastRecvTick;
	};
	struct st_SECTOR
	{
		short shSectorX, shSectorY;
	};
	struct st_SECTOR_AROUND
	{
		int iCount;
		st_SECTOR Around[9];
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 소멸자
	//
	//////////////////////////////////////////////////////////////////////////
	CChatServer();
	virtual ~CChatServer();

	bool InitPDH();	// 생성자에서 호출

	bool Start();
	void Stop();

	// 1000ms마다 호출
	void Monitoring();

private:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnConnectRequest		: Accept 직후, true/false 접속 허용/거부
	// OnClientJoin			: Accept 접속처리 완료 후, 유저 접속 관련
	// OnClientLeave		: Disconnect 후, 유저 정리
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnError				: 에러 발생 후
	//////////////////////////////////////////////////////////////////////////
	bool OnConnectRequest(WCHAR* wszIP, int iPort);
	void OnClientJoin(UINT64 SessionID);
	void OnClientLeave(UINT64 SessionID);
	void OnRecv(UINT64 SessionID, CNPacket * pPacket);	// 패킷 수신 완료 후
	void OnSend(UINT64 SessionID, int iSendSize);	// 패킷 송신 완료 후
	void OnError(int iErrCode, WCHAR * wszErr);

	//////////////////////////////////////////////////////////////////////////
	// Thread
	//
	//////////////////////////////////////////////////////////////////////////
	static unsigned int __stdcall UpdateThread(LPVOID pCChatServer);
	unsigned int __stdcall UpdateThread_Process();

	// Update Thread
	void proc_MSG_JOIN(st_MESSAGE * pMessage);	// stPlayer 생성
	void proc_MSG_LEAVE(st_MESSAGE * pMessage);	// stPlayer 해제
	void proc_MSG_PACKET(st_MESSAGE * pMessage);

	//////////////////////////////////////////////////////////////////////////
	// Sector
	//
	////////////////////////////////////////////////////////////////////////// 
	bool GetSectorAround(short shSectorX, short shSectorY, st_SECTOR_AROUND *pSectorAround);
	bool SetSector(st_PLAYER* pPlayer, short shSectorX, short shSectorY);
	bool EnterSector(st_PLAYER* pPlayer, short shSectorX, short shSectorY);
	bool LeaveSector(st_PLAYER* pPlayer);

	// Send
	void SendPacket_Around(st_PLAYER* pPlayer, CNPacket *pPacket, bool bSendMe = true);

	//////////////////////////////////////////////////////////////////////////
	// Response
	//
	////////////////////////////////////////////////////////////////////////// 
	st_PLAYER* SearchPlayer(UINT64 iSessionID);

	void proc_PACKET_CS_CHAT_REQ_LOGIN(UINT64 iSessionID, CNPacket *pPacket);
	void proc_PACKET_CS_CHAT_REQ_SECTOR_MOVE(UINT64 iSessionID, CNPacket *pPacket);
	void proc_PACKET_CS_CHAT_REQ_MESSAGE(UINT64 iSessionID, CNPacket *pPacket);
	void proc_PACKET_CS_CHAT_REQ_HEARTBEAT(UINT64 iSessionID, CNPacket *pPacket);

	void mp_PACKET_CS_CHAT_RES_LOGIN(CNPacket *pBuffer, BYTE byStatus, INT64 iAccountNo);
	void mp_PACKET_CS_CHAT_RES_SECTOR_MOVE(CNPacket *pBuffer, INT64 iAccountNo, WORD wSectorX, WORD wSectorY);
	void mp_PACKET_CS_CHAT_RES_MESSAGE(CNPacket *pBuffer, INT64 iAccountNo, WCHAR *szID, WCHAR *szNickname, WORD wMessageLen, WCHAR *szMessage);


private:
	// Thread
	HANDLE	_hUpdateThread;
	HANDLE	_hUpdateEvent;
	bool	_bShutdown;

	// Player
	CLFMemoryPool_TLS<st_PLAYER>	_PlayerPool;
	map<UINT64, st_PLAYER*>		_PlayerMap;	// SessionID, st_PLAYER*

	// Sector
	list<st_PLAYER*>			_Sector[en_SECTOR_MAX_Y][en_SECTOR_MAX_X];

	// Message
	CLFMemoryPool_TLS<st_MESSAGE>	_MessagePool;
	CLFQueue<st_MESSAGE*>		_MessageQ;

	// Client
	CMonitorClient* _pMonitorClient;
	CLoginClient*	_pLoginClient;

public:
	//----------------------------------------------------------------------------
	// Monitor
	//----------------------------------------------------------------------------
	LONG64	_lMonitor_PlayerCnt;
	LONG64	_lMonitor_SessionMissCnt;
	LONG64	_lMonitor_UpdateTps;

	// CPU Usage
	CCpuUsage		_CPUTime;
	// PDH
	PDH_HQUERY		_PDHQuery;
	PDH_HCOUNTER	_pdh_Counter_PrivateBytes;
};
#endif