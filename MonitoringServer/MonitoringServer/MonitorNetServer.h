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
	// 이더넷 하나에 대한 Send,Recv PDH 쿼리 정보
	//--------------------------------------------
	struct st_ETHERNET
	{
		bool			_bUse;
		WCHAR			_szName[128];

		PDH_HCOUNTER	_pdh_Counter_Network_RecvBytes;
		PDH_HCOUNTER	_pdh_Counter_Network_SendBytes;
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 소멸자
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
	// 스레드에서 1초(en_WORKER_SLEEP)마다 호출
	//////////////////////////////////////////////////////////////////////////
	void UpdateClient();	// 모니터링 클라이언트에 데이터 전송
	void UpdatePDH();		// PDH 갱신
	void UpdateDB();		// DB 저장
	void TimeOut_LanClient();

private:
	void SendData(BYTE byServerType, BYTE byDataType, int iData); // UpdateClient() 내부 호출
	bool InitPDH();	// 생성자에서 호출


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
	// DBWriter, SendData, PdhUpdate / 메인스레드로 옮김
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
	st_ETHERNET		_Ethernet[df_PDH_ETHERNET_MAX];	// 랜카드 별 PDH 정보
	PDH_HQUERY		_PDHQuery;

	PDH_HCOUNTER	_pdh_Counter_ProcessorTime;		 // 하드웨어 CPU 사용률 전체
	double			_pdh_Value_ProcessorTime;

	PDH_HCOUNTER	_pdh_Counter_AvailableMBytes;	 // 하드웨어 사용가능 메모리
	double			_pdh_Value_AvailableMBytes;
	
	double			_pdh_Value_Network_RecvBytes;	 // 하드웨어 이더넷 수신 바이트
	
	double			_pdh_Value_Network_SendBytes;	 // 하드웨어 이더넷 송신 바이트
	
	PDH_HCOUNTER	_pdh_Counter_NonpagedBytes;		 // 하드웨어 논페이지 메모리 사용량
	double			_pdh_Value_NonpagedBytes;


};

#endif