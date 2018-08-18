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

	// 1000ms���� ȣ��
	void	Monitoring();

private:
	// AUTH ��� ������Ʈ �̺�Ʈ ����ó����
	virtual	void	OnAuth_Process(void);

	// GAME ��� ������Ʈ �̺�Ʈ ����ó����
	virtual	void	OnGame_Process(void);

	bool InitPDH();	// �����ڿ��� ȣ��

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
	//  Auth ���� - AuthPacket �ۼ��� - Auth ���� - Game ���� - GamePacket �ۼ��� - Game ���� - Release 
	//
	// OnAuth_ClientJoin	: ó�� ����. ���� ������ �ʱ�ȭ
	// OnAuth_ClientLeave	: ���� ����. ���� ������ ����
	// OnAuth_Packet		: �α��� ��Ŷ ó��.
	// OnGame_ClientJoin	: ���� ����. ���� ������ �ʱ�ȭ
	// OnGame_ClientLeave	: ���� ����. ���� ������ ����
	// OnGame_Packet		: ���� ��Ŷ ó��.
	// OnGame_ClientRelease	: ���� ���� ����
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