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
		// st_LOGIN_INFO ���� ����
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
	// OnClientJoin			: ���� ���� ����
	// OnClientLeave		: ���� ���� ���� ����
	// OnRecv				: ��Ŷ ���� ��, ��Ŷ ó��
	// OnSend				: ��Ŷ �۽� ��
	// OnWorkerThreadBegin	: WorkerThread GQCS ����
	// OnWorkerThreadEnd	: WorkerThread Loop ���� ��
	// OnError				: ���� �߻� ��
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
	void			LoginMap_Timeout(); // ���Ӹ� �ϰ� ä�� ������ �������� �ʴ� ���� ����
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