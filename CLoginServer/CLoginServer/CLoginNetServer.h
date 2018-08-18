#ifndef __LOGIN_SERVER__
#define __LOGIN_SERVER__

#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CNetServer.h"
#include "CMonitorClient.h"
#include "CLoginLanServer.h"
#include "CommonProtocol.h"
#include "CCpuUsage.h"
#include <Pdh.h>
#pragma comment(lib, "pdh.lib")
#include <list>
#include "CDBAccount.h"

using namespace std;
using namespace mylib;

class CLoginServer : public CNetServer
{
public:
	enum en_SERVER_CONFIG
	{
		// Message Type
		en_MSG_JOIN = 0,
		en_MSG_LEAVE,
		en_MSG_PACKET,
		en_MSG_HEARTBEAT,

		// Heartbeat timeout
		en_TIME_OUT = 60000
	};
	struct st_PLAYER
	{
		UINT64	iSessionID;
		INT64	iAccountNo;
		WCHAR	szID[20];
		WCHAR	szNickname[20];
		char	szSessionKey[64];

		BYTE	byStatus;

		ULONGLONG lLoginReqTick;
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 소멸자
	//
	//////////////////////////////////////////////////////////////////////////
	CLoginServer();
	virtual ~CLoginServer();

	bool InitPDH();	// 생성자에서 호출

	bool Start();
	void Stop();

	// 1000ms마다 호출
	void Monitoring();

private:
	st_PLAYER * SearchPlayer(UINT64 iSessionID);

	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	// Client to LoginServer
	/* NetServer ::*/void proc_PACKET_CS_LOGIN_REQ_LOGIN(UINT64 iSessionID, CNPacket *pPacket);
	// LanServer ::  void proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(INT64 iAccountNo, CHAR* szSessionKey, INT64 iParameter);

	// Server to Server
	// LanServer ::  void proc_PACKET_SS_RES_NEW_CLIENT_LOGIN(UINT64 iSessionID, CNPacket *pPacket);
	/* NetServer ::*/void comp_PACKET_SS_RES_NEW_CLIENT_LOGIN(BYTE byServerType, INT64 iAccountNo, INT64 iParameter);
	void mpResLogin(CNPacket * pBuffer, INT64 iAccountNo, BYTE byStatus, WCHAR* szID, WCHAR* szNick);


protected:
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
	virtual bool OnConnectRequest(WCHAR* wszIP, int iPort);
	virtual void OnClientJoin(UINT64 SessionID);
	virtual void OnClientLeave(UINT64 SessionID);
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);
	virtual void OnHeartBeat();


private:
	// Thread
	HANDLE	_hUpdateThread;
	HANDLE	_hUpdateEvent;
	bool	_bShutdown;
	bool	_bControlMode;

	// Player
	CLFMemoryPool<st_PLAYER>	_PlayerPool;
	list<st_PLAYER*>			_PlayerList;
	SRWLOCK						_srwPlayerLock;

	// Monitoring
	CMonitorClient* _pMonitorClient;

	LONG64		_lLoginRequestCnt;
	LONG64		_lLoginSuccessTps;
	ULONGLONG	_lLoginSuccessTime_Min;
	ULONGLONG	_lLoginSuccessTime_Max;
	ULONGLONG	_lLoginSuccessTime_Cnt;

	// CPU Usage
	CCpuUsage		_CPUTime;
	// PDH
	PDH_HQUERY		_PDHQuery;
	PDH_HCOUNTER	_pdh_Counter_PrivateBytes;

	// Server to Server
	friend class CLoginLanServer;
	CLoginLanServer* _pLoginLanServer;
	bool		_bConnectChatServer;

	// DB
	CDBAccount* _pDBAccount;
};

#endif