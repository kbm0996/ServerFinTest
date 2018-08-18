#ifndef __MONITOR_NETSERVER_LOG__
#define __MONITOR_NETSERVER_LOG__
#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CNetServer.h"
#include "MonitorProtocol.h"
#include "MonitorLanServer.h"
#include "CCpuUsage.h"
#include "CConfig.h"
#include <Pdh.h>
#pragma comment(lib, "pdh.lib")
#include <list>
#include "CDBConnector_TLS.h"
using namespace mylib;
using namespace std;

#define df_PDH_ETHERNET_MAX 8

class CMonitorLanServer;
class CMonitorNetServer : public CNetServer
{
public:
	enum en_SERVER_CONFIG
	{
		en_DBSAVE_INTERVAL = 600000,

		en_WORKER_SLEEP = 1000
	};

	struct st_PLAYER
	{
		st_PLAYER()
		{
			szLoginSessionKey[32] = { 0, };
		}
		UINT64	iSessionID;
		INT64	iAccountNo;
		char	szLoginSessionKey[32];

		ULONGLONG lLoginReqTick;
	};

	//--------------------------------------------
	// �̴��� �ϳ��� ���� Send,Recv PDH ���� ����
	//--------------------------------------------
	struct st_ETHERNET
	{
		bool			_bUse;
		WCHAR			_szName[128];

		PDH_HCOUNTER	_pdh_Counter_Network_RecvBytes;
		PDH_HCOUNTER	_pdh_Counter_Network_SendBytes;
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �Ҹ���
	//
	//////////////////////////////////////////////////////////////////////////
	CMonitorNetServer();
	virtual ~CMonitorNetServer();

	bool Start();
	void Stop();

	//////////////////////////////////////////////////////////////////////////
	// Print Monitoring Data
	//
	//////////////////////////////////////////////////////////////////////////
	// Console
	void Monitoring_Console();

	
public:
	//////////////////////////////////////////////////////////////////////////
	// WorkerThread Function
	//
	// �����忡�� 1��(en_WORKER_SLEEP)���� ȣ��
	//////////////////////////////////////////////////////////////////////////
	void UpdateClient();	// ����͸� Ŭ���̾�Ʈ�� ������ ����
	void UpdatePDH();		// PDH ����
	void UpdateDB();		// DB ����
	void TimeOut_LanClient();

private:
	void SendData(BYTE byServerType, BYTE byDataType, int iData); // UpdateClient() ���� ȣ��
	bool InitPDH();	// �����ڿ��� ȣ��


protected:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnConnectRequest		: Accept ����, true/false ���� ���/�ź�
	// OnClientJoin			: Accept ����ó�� �Ϸ� ��, ���� ���� ����
	// OnClientLeave		: Disconnect ��, ���� ����
	// OnRecv				: ��Ŷ ���� ��, ��Ŷ ó��
	// OnSend				: ��Ŷ �۽� ��
	// OnError				: ���� �߻� ��
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnConnectRequest(WCHAR* wszIP, int iPort);
	virtual void OnClientJoin(UINT64 SessionID);
	virtual void OnClientLeave(UINT64 SessionID);
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);
	virtual void OnHeartBeat();

private:
	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	st_PLAYER * SearchPlayer(UINT64 iSessionID);

	// OnRecv
	void proc_PACKET_CS_MONITOR_TOOL_REQ_LOGIN(UINT64 SessionID, CNPacket * pPacket);
	void mp_PACKET_CS_MONITOR_TOOL_RES_LOGIN(CNPacket * pPacket, BYTE byStatus);

	//////////////////////////////////////////////////////////////////////////
	// Thread
	//
	//////////////////////////////////////////////////////////////////////////
	// DBWriter, SendData, PdhUpdate / ���ν������ �ű�
	///static unsigned int __stdcall WorkerThread(LPVOID pCNetServer);
	///unsigned int __stdcall WorkerThread_Process();

private:
	CMonitorLanServer* _pLanServer;

	// Player
	CLFMemoryPool<st_PLAYER>	_PlayerPool;
	list<st_PLAYER*>			_PlayerList;
	SRWLOCK						_srwPlayerLock;

	// WorkerThread
	CDBConnector_TLS*			_pDBConnector;
	///HANDLE						_hWorkerThread;
	bool						_bShutdown;

	// CPU Usage
	CCpuUsage					_CPUTime;

	//////////////////////////////////////////////////////////////////////////
	// PDH
	//
	//////////////////////////////////////////////////////////////////////////
	st_ETHERNET		_Ethernet[df_PDH_ETHERNET_MAX];	// ��ī�� �� PDH ����
	PDH_HQUERY		_PDHQuery;

	PDH_HCOUNTER	_pdh_Counter_ProcessorTime;		 // �ϵ���� CPU ���� ��ü
	double			_pdh_Value_ProcessorTime;

	PDH_HCOUNTER	_pdh_Counter_AvailableMBytes;	 // �ϵ���� ��밡�� �޸�
	double			_pdh_Value_AvailableMBytes;
	
	double			_pdh_Value_Network_RecvBytes;	 // �ϵ���� �̴��� ���� ����Ʈ
	
	double			_pdh_Value_Network_SendBytes;	 // �ϵ���� �̴��� �۽� ����Ʈ
	
	PDH_HCOUNTER	_pdh_Counter_NonpagedBytes;		 // �ϵ���� �������� �޸� ��뷮
	double			_pdh_Value_NonpagedBytes;


};

#endif