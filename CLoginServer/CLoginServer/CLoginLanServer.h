#ifndef __LOGIN_LAN_SERVER__
#define __LOGIN_LAN_SERVER__

#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CLanServer.h"
#include "CLoginNetServer.h"
#include <list>
#include <map>

using namespace std;
using namespace mylib;

class CLoginServer;

class CLoginLanServer : public CLanServer
{
public:
	struct st_SERVER
	{
		UINT64	iSessionID;
		BYTE	byServerType;	
		WCHAR	szServerName[32];	
	};
	CLoginLanServer(CLoginServer* pLoginServer);
	virtual ~CLoginLanServer();

private:
	st_SERVER* SearchServer(UINT64 iSessionID);

	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	// Server to Server
	void ReqLoginServerLogin(UINT64 iSessionID, CNPacket *pPacket);

	// 다른 서버로부터 Client 최종 접속 통지
	/* LanServer ::*/void proc_PACKET_SS_RES_NEW_CLIENT_LOGIN(UINT64 iSessionID, CNPacket *pPacket);
	// NetServer ::  void comp_PACKET_SS_RES_NEW_CLIENT_LOGIN(BYTE byServerType, INT64 iAccountNo, INT64 iParameter);

	// NetServer to LanServer
	// NetServer ::  void proc_PACKET_CS_LOGIN_REQ_LOGIN(UINT64 iSessionID, CNPacket *pPacket);
	/* LanServer ::*/void proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(INT64 iAccountNo, CHAR* szSessionKey, INT64 iParameter);

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
	virtual void OnClientLeave(UINT64 SessionID);;
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);

private:
	// Player
	list<st_SERVER*>	_ServerList;
	SRWLOCK				_srwServerLock;

	friend class CLoginServer;
	CLoginServer* _pLoginNetServer;
};

#endif