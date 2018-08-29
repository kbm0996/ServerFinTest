#ifndef __LOGIN_CLIENT_H__
#define __LOGIN_CLIENT_H__
#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CLanClient.h"
#include "CChatServer.h"
#include <list>
#include <map>

using namespace std;
using namespace mylib;

class CChatServer;
class CLoginClient : public CLanClient
{
	enum en_LOGIN_CLIENT
	{
		// st_LOGIN_INFO 삭제 간격
		en_INFO_TIMEOUT = 60000
	};

	struct st_LOGIN_INFO
	{
		INT64	iAccountNo;
		char	szSessionKey[64];
		INT64	iParameter;
		ULONGLONG lLastRecvTick;
	};

public:
	//////////////////////////////////////////////////////////////////////////
	// Server Control
	//
	//////////////////////////////////////////////////////////////////////////
	// Constructor & Destructor
	CLoginClient(CChatServer* pChatServer);
	virtual ~CLoginClient();

private:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnClientJoin			: 서버 연결 직후
	// OnClientLeave		: 서버 연결 끊긴 이후
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnWorkerThreadBegin	: WorkerThread GQCS 직후
	// OnWorkerThreadEnd	: WorkerThread Loop 종료 후
	// OnError				: 에러 발생 후
	//////////////////////////////////////////////////////////////////////////
	virtual void OnClientJoin();
	virtual void OnClientLeave();
	virtual void OnRecv(CNPacket * pPacket);
	virtual void OnSend(int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);

	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	void proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(CNPacket *pPacket);

	//////////////////////////////////////////////////////////////////////////
	// LoginList
	//
	//////////////////////////////////////////////////////////////////////////
	st_LOGIN_INFO*	LoginMap_Find(INT64 iAccountNo);
	bool			LoginMap_Add(st_LOGIN_INFO* pAddInfo);
	bool			LoginMap_Remove(INT64 iAccountNo, char* szSessionKey);
	void			LoginMap_Timeout(); // 접속만 하고 채팅 서버로 접속하지 않는 유저 제거
	int				LoginMap_Size();

private:
	friend class CChatServer;
	CChatServer* _pChatServer;
	
	SRWLOCK							_srwLoginMap;
	map<INT64, st_LOGIN_INFO*>		_LoginMap;
	CLFMemoryPool<st_LOGIN_INFO>*	_LogininfoPool;

	bool _bConnection;
};
#endif