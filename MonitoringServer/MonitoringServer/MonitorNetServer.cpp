#include "MonitorNetServer.h"

using namespace mylib;

CMonitorNetServer::CMonitorNetServer()
{
	LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG, L"MONITOR_SERVER_LOG");
	///CNPacket::PacketPool();
	if (!InitPDH())
		CRASH();

	_pLanServer = nullptr;
	_bShutdown = false;
}

CMonitorNetServer::~CMonitorNetServer()
{
	if(_pLanServer != nullptr)
		delete _pLanServer;
}

bool CMonitorNetServer::Start()
{
	_pLanServer = new CMonitorLanServer();
	if (!_pLanServer->Start(CConfig::_szLanBindIP, CConfig::_iLanBindPort, 1, false, 10))
	{
		LOG(L"MONITOR_NETSERVER_LOG", LOG_ERROR, L"LanServer Start Error");
		return false;
	}

	if (!CNetServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, 1, false, 10, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
	{
		LOG(L"MONITOR_NETSERVER_LOG", LOG_ERROR, L"NetServer Start Error");
		return false;
	}

	_pDBConnector = new CDBConnector_TLS(CConfig::_szDBIP, CConfig::_szDBAccount, CConfig::_szDBPassword, CConfig::_szDBName, CConfig::_iDBPort);
	///_hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);

	return true;
}

void CMonitorNetServer::Stop()
{
	_bShutdown = true;
	///WaitForSingleObject(_hWorkerThread, INFINITE);

	CNetServer::Stop();
	_pLanServer->Stop();
}

bool CMonitorNetServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CMonitorNetServer::OnClientJoin(UINT64 SessionID)
{
	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	//pPlayer->iAccountNo = 0;
	pPlayer->iSessionID = SessionID;
	pPlayer->lLoginReqTick = -1;

	AcquireSRWLockExclusive(&_srwPlayerLock);
	_PlayerList.push_back(pPlayer);
	ReleaseSRWLockExclusive(&_srwPlayerLock);
}

void CMonitorNetServer::OnClientLeave(UINT64 SessionID)
{
	AcquireSRWLockExclusive(&_srwPlayerLock);
	st_PLAYER* pPlayer = nullptr;
	for (auto Iter = _PlayerList.begin(); Iter != _PlayerList.end(); Iter++)
	{
		pPlayer = *Iter;
		if (pPlayer->iSessionID == SessionID)
		{
			_PlayerList.erase(Iter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&_srwPlayerLock);

	pPlayer->lLoginReqTick = -1;
	_PlayerPool.Free(pPlayer);
}

void CMonitorNetServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		proc_PACKET_CS_MONITOR_TOOL_REQ_LOGIN(SessionID, pPacket);
		break;
	default:
		LOG(L"MONITOR_NETSERVER_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		break;
	}
}

void CMonitorNetServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

void CMonitorNetServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CMonitorNetServer::OnHeartBeat()
{
}

void CMonitorNetServer::Monitoring_Console()
{
	wprintf(L"===========================================\n");
	wprintf(L" Monitoring Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - HW CPU Usage 	: %.1f\n", _pdh_Value_ProcessorTime);
	wprintf(L" - HW Available Memory	: %.1fM\n", _pdh_Value_AvailableMBytes);
	wprintf(L" - HW NonPaged Memory	: %.1fM\n", _pdh_Value_NonpagedBytes);
	wprintf(L" - Ethernet RecvBytes	: %.3fK\n", _pdh_Value_Network_RecvBytes);
	wprintf(L" - Ethernet SendBytes	: %.3fK\n", _pdh_Value_Network_SendBytes);
	PrintState(true);
	if (_pLanServer->_bOnLoginServer)
	{
		wprintf(L"===========================================\n");
		wprintf(L" Login Server\n");
		wprintf(L"===========================================\n");
		wprintf(L" - SESSION		: %d\n", _pLanServer->_iSessionAll_Login);
		wprintf(L" - LoginSuccess TPS	: %d\n", _pLanServer->_iLoginSuccessTps_Login);
		wprintf(L"\n");
		wprintf(L" - Packet Pool		: %d\n", _pLanServer->_iPacketPool_Login);
		wprintf(L" - CPU Usage		: %d\n", _pLanServer->_iCpuUsage_Login);
		wprintf(L" - Commit Memory	: %dM\n", _pLanServer->_iCommitMemory_Login);
	}
	if (_pLanServer->_bOnChatServer)
	{
		wprintf(L"===========================================\n");
		wprintf(L" Chat Server\n");
		wprintf(L"===========================================\n");
		wprintf(L" - SESSION		: %d\n", _pLanServer->_iSessionAll_Chat);
		wprintf(L" - PLAYER		: %d\n", _pLanServer->_iSessionLogin_Chat);
		wprintf(L" - MSG Pool		: %d\n", _pLanServer->_iMsgPool_Chat);
		wprintf(L" - MSG TPS		: %d\n", _pLanServer->_iMsgTps_Chat);
		wprintf(L"\n");
		wprintf(L" - Packet Pool		: %d\n", _pLanServer->_iPacketPool_Chat);
		wprintf(L" - CPU Usage		: %d\n", _pLanServer->_iCpuUsage_Chat);
		wprintf(L" - Commit Memory	: %dM\n", _pLanServer->_iCommitMemory_Chat);
	}
	if (_pLanServer->_bOnGameServer)
	{
		wprintf(L"===========================================\n");
		wprintf(L" Echo Server\n");
		wprintf(L"===========================================\n");
		wprintf(L" - ALL Mode		: %d\n", _pLanServer->_iSessionAll_Game);
		wprintf(L" - Auth Mode		: %d\n", _pLanServer->_iSessionAuth_Game);
		wprintf(L" - Game Mode		: %d\n", _pLanServer->_iSessionGame_Game);
		wprintf(L"\n");
		wprintf(L" - Auth Update FPS	: %d\n", _pLanServer->_iAuthFps_Game);
		wprintf(L" - Game Update FPS	: %d\n", _pLanServer->_iGameFps_Game);
		wprintf(L"\n");
		wprintf(L" - Packet Pool		: %d\n", _pLanServer->_iPacketPool_Game);
		wprintf(L" - CPU Usage		: %d\n", _pLanServer->_iCpuUsage_Game);
		wprintf(L" - Commit Memory	: %dM\n", _pLanServer->_iCommitMemory_Game);
	}
}

CMonitorNetServer::st_PLAYER * CMonitorNetServer::SearchPlayer(UINT64 iSessionID)
{
	AcquireSRWLockExclusive(&_srwPlayerLock);
	st_PLAYER* pPlayer = nullptr;
	auto Iter = _PlayerList.begin();
	for (; Iter != _PlayerList.end(); Iter++)
	{
		pPlayer = (*Iter);
		if (pPlayer->iSessionID == iSessionID)
			break;
	}
	if (Iter == _PlayerList.end())
		pPlayer = nullptr;
	ReleaseSRWLockExclusive(&_srwPlayerLock);
	return  pPlayer;
}

void CMonitorNetServer::proc_PACKET_CS_MONITOR_TOOL_REQ_LOGIN(UINT64 SessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 모니터링 클라이언트(툴) 이 모니터링 서버로 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		char	LoginSessionKey[32]		// 로그인 인증 키. (이는 모니터링 서버에 고정값으로 보유)
	//										// 각 모니터링 툴은 같은 키를 가지고 들어와야 함
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,
	st_PLAYER *pPlayer = SearchPlayer(SessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"MONITOR_NETSERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}
	BYTE byStatus = 1;

	pPacket->GetData((char*)pPlayer->szLoginSessionKey, sizeof(pPlayer->szLoginSessionKey));

	if (memcmp(pPlayer->szLoginSessionKey, CConfig::_szLoginSessionKey, sizeof(pPlayer->szLoginSessionKey)) != 0)
	{
		byStatus = 0;
		LOG(L"MONITOR_NETSERVER_LOG", LOG_WARNG, L"Session %d szLoginKey Error", SessionID);
	}

	//------------------------------------------------------------
	// 모니터링 클라이언트(툴) 모니터링 서버로 로그인 응답
	// 로그인에 실패하면 0 보내고 끊어버림
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status					// 로그인 결과 0 / 1 
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,
	CNPacket *Packet = CNPacket::Alloc();
	mp_PACKET_CS_MONITOR_TOOL_RES_LOGIN(Packet, byStatus);
	
	if (byStatus == 1)
	{
		SendPacket(SessionID, Packet);
	}
	else
	{
		SendPacket_Disconnect(SessionID, Packet);
	}

	Packet->Free();
}

//unsigned int CMonitorNetServer::WorkerThread(LPVOID pCNetServer)
//{
//	return ((CMonitorNetServer*)pCNetServer)->WorkerThread_Process();
//}
//
//unsigned int CMonitorNetServer::WorkerThread_Process()
//{
//	ULONGLONG lUpdateTick = GetTickCount64();
//	while (!_bShutdown)
//	{
//		UpdatePDH();
//		UpdateClient();
//		TimeOut_LanClient();
//		if (GetTickCount64() - lUpdateTick >= en_DBSAVE_INTERVAL)
//		{
//			UpdateDB();
//			lUpdateTick = GetTickCount64();
//		}
//		Sleep(en_WORKER_SLEEP);
//	}
//	LOG(L"MONITOR_NETSERVER_LOG", LOG_DEBUG, L"WorkerThread Exit");
//	return 0;
//}

void CMonitorNetServer::UpdateClient()
{
	//dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL = 1,                    // 하드웨어 CPU 사용률 전체
	//dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY,                 // 하드웨어 사용가능 메모리
	//dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV,                     // 하드웨어 이더넷 수신 바이트
	//dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND,                     // 하드웨어 이더넷 송신 바이트
	//dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY,                  // 하드웨어 논페이지 메모리 사용량
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_MASTER_SERVER_ON, TRUE);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL, _pdh_Value_ProcessorTime);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY, _pdh_Value_AvailableMBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, _pdh_Value_Network_RecvBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND, _pdh_Value_Network_SendBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY, _pdh_Value_NonpagedBytes);

	if (_pLanServer->_bOnLoginServer)
	{
		//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // 매치메이킹 서버 ON
		//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // 매치메이킹 CPU 사용률 (커널 + 유저)
		//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // 매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
		//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // 매치메이킹 패킷풀 사용량
		//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // 매치메이킹 접속 세션
		//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // 매치메이킹 접속 유저 (로그인 성공 후)
		//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // 매치메이킹 방 배정 성공 수 (초당)
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_SERVER_ON, _pLanServer->_bOnLoginServer);
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_CPU, _pLanServer->_iCpuUsage_Login);
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT, _pLanServer->_iCommitMemory_Login);
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL, _pLanServer->_iPacketPool_Login);
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_SESSION, _pLanServer->_iSessionAll_Login);
		//SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_PLAYER, _pLanServer->_iSessionSuccess_Login);
		SendData(dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS, _pLanServer->_iLoginSuccessTps_Login);

	}
	if (_pLanServer->_bOnGameServer)
	{
		//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // 배틀서버 ON
		//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // 배틀서버 CPU 사용률 (커널 + 유저)
		//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
		//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // 배틀서버 패킷풀 사용량
		//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // 배틀서버 Auth 스레드 초당 루프 수
		//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // 배틀서버 Game 스레드 초당 루프 수
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // 배틀서버 접속 세션전체
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // 배틀서버 Auth 스레드 모드 인원
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // 배틀서버 Game 스레드 모드 인원
		//dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // 배틀서버 대기방 수
		//dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // 배틀서버 플레이방 수
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, _pLanServer->_bOnGameServer);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_CPU, _pLanServer->_iCpuUsage_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, _pLanServer->_iCommitMemory_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, _pLanServer->_iPacketPool_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, _pLanServer->_iAuthFps_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, _pLanServer->_iGameFps_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, _pLanServer->_iSessionAll_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, _pLanServer->_iSessionAuth_Game);
		SendData(dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, _pLanServer->_iSessionGame_Game);
	}
	if (_pLanServer->_bOnChatServer)
	{
		//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // 채팅서버 ON
		//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // 채팅서버 CPU 사용률 (커널 + 유저)
		//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
		//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // 채팅서버 패킷풀 사용량
		//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // 채팅서버 접속 세션전체
		//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // 채팅서버 로그인을 성공한 전체 인원
		//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL,
		//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_ON, _pLanServer->_bOnChatServer);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_CPU, _pLanServer->_iCpuUsage_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT, _pLanServer->_iCommitMemory_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, _pLanServer->_iPacketPool_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SESSION, _pLanServer->_iSessionAll_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_PLAYER, _pLanServer->_iSessionLogin_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, _pLanServer->_iMsgPool_Chat);
		SendData(dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS, _pLanServer->_iMsgTps_Chat);
	}
}

void CMonitorNetServer::UpdatePDH()
{
	/////////////////////////////////////////////////////////////////////////////////
	// CPUUsage
	/////////////////////////////////////////////////////////////////////////////////
	// CPUUsage 갱신
	_CPUTime.UpdateCpuTime();

	/////////////////////////////////////////////////////////////////////////////////
	// PDH
	/////////////////////////////////////////////////////////////////////////////////
	// PDH 쿼리 갱신
	PdhCollectQueryData(_PDHQuery);
	PDH_STATUS Status;
	PDH_FMT_COUNTERVALUE CounterValue;
	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (_Ethernet[iCnt]._bUse)
		{
			Status = PdhGetFormattedCounterValue(_Ethernet[iCnt]._pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
			if (Status == 0)
				_pdh_Value_Network_RecvBytes = CounterValue.doubleValue / 1024;

			Status = PdhGetFormattedCounterValue(_Ethernet[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
			if (Status == 0)
				_pdh_Value_Network_SendBytes = CounterValue.doubleValue / 1024;
		}
	}
	Status = PdhGetFormattedCounterValue(_pdh_Counter_ProcessorTime, PDH_FMT_DOUBLE, NULL, &CounterValue);
	if (Status == 0)
		_pdh_Value_ProcessorTime = CounterValue.doubleValue;

	Status = PdhGetFormattedCounterValue(_pdh_Counter_AvailableMBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
	if (Status == 0)
		_pdh_Value_AvailableMBytes = CounterValue.doubleValue;

	Status = PdhGetFormattedCounterValue(_pdh_Counter_NonpagedBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
	if (Status == 0)
		_pdh_Value_NonpagedBytes = CounterValue.doubleValue / 1024 / 1024;
}

void CMonitorNetServer::UpdateDB()
{
	_pDBConnector->Query(L"INSERT INTO `status_server`.`hardware` (`date`, `CPUUsage`, `AvailableMBytes`, `NonPagedBytes`) VALUES(NOW(), '%d', '%d', '%d');", (int)_pdh_Value_ProcessorTime, (int)_pdh_Value_AvailableMBytes, (int)_pdh_Value_NonpagedBytes);
	if (_pLanServer->_bOnGameServer)
	{
		_pDBConnector->Query(L"INSERT INTO `status_server`.`battle` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `AuthFPS`, `GameFPS`, `SessionAll`, `SessionAuth`, `SessionGame`) VALUES (NOW(), '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');", _pLanServer->_iCpuUsage_Game, _pLanServer->_iCommitMemory_Game, _pLanServer->_iPacketPool_Game, _pLanServer->_iAuthFps_Game, _pLanServer->_iGameFps_Game, _pLanServer->_iSessionAll_Game, _pLanServer->_iSessionAuth_Game, _pLanServer->_iSessionGame_Game);
	}
	if (_pLanServer->_bOnChatServer)
	{
		_pDBConnector->Query(L"INSERT INTO `status_server`.`chat` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `SessionLogin`) VALUES (NOW(), '%d', '%d', '%d', '%d', '%d');", _pLanServer->_iCpuUsage_Chat, _pLanServer->_iCommitMemory_Chat, _pLanServer->_iPacketPool_Chat, _pLanServer->_iSessionAll_Chat, _pLanServer->_iSessionLogin_Chat);
	}
	if (_pLanServer->_bOnLoginServer)
	{
		_pDBConnector->Query(L"INSERT INTO `status_server`.`login` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `LoginSuccessTPS`) VALUES (NOW(), '%d', '%d', '%d', '%d', '%d');", _pLanServer->_iCpuUsage_Login, _pLanServer->_iCommitMemory_Login, _pLanServer->_iPacketPool_Login, _pLanServer->_iSessionAll_Login, _pLanServer->_iLoginSuccessTps_Login);
	}
}

void CMonitorNetServer::TimeOut_LanClient()
{
	_pLanServer->ServerList_Timeout();
}

void CMonitorNetServer::SendData(BYTE byServerType, BYTE byDataType, int iData)
{
	//------------------------------------------------------------
	// 모니터링 서버가 모니터링 클라이언트(툴) 에게 모니터링 데이터 전송
	//
	// 모니터링 서버는 모든 모니터링 클라이언트에게 모든 데이터를 뿌려준다.
	//
	// 데이터를 절약하기 위해서는 초단위로 모든 데이터를 묶어서 30~40개의 모니터링 데이터를 하나의 패킷으로 만드는게
	// 좋으나  여러가지 생각할 문제가 많으므로 그냥 각각의 모니터링 데이터를 개별적으로 전송처리 한다.
	//
	//	{
	//		WORD	Type
	//		
	//		BYTE	ServerNo				// 서버 No
	//		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
	//		int		DataValue				// 해당 데이터 수치.
	//		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
	//										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
	//										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE,

	CNPacket* pSendPacket = CNPacket::Alloc();
	*pSendPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	*pSendPacket << byServerType;
	*pSendPacket << byDataType;
	*pSendPacket << iData;
	*pSendPacket << (int)time(NULL);

	AcquireSRWLockExclusive(&_srwPlayerLock);
	for (auto Iter = _PlayerList.begin(); Iter != _PlayerList.end(); Iter++)
	{
		SendPacket((*Iter)->iSessionID, pSendPacket);
	}
	ReleaseSRWLockExclusive(&_srwPlayerLock);
	pSendPacket->Free();
}

bool CMonitorNetServer::InitPDH()
{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// PDH
	// 배틀서버 메모리 유저 커밋 사용량 (Private) MByte 
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Create a query.
	if (PdhOpenQuery(NULL, NULL, &_PDHQuery) != ERROR_SUCCESS)
		return false;

	int iCnt = 0;
	WCHAR *szCur = NULL;
	WCHAR *szCounters = NULL;
	WCHAR *szInterfaces = NULL;
	DWORD dwCounterSize = 0;
	DWORD dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };
	// 버퍼의 길이 구하기 : OutBuffer 인자들을 모두 NULL로
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return false;
	}

	iCnt = 0;
	szCur = szInterfaces;

	// szInterfaces 에서 문자열 단위로 끊으면서 , 이름을 복사받는다.
	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_Ethernet[iCnt]._bUse = true;
		_Ethernet[iCnt]._szName[0] = L'\0';

		wcscpy_s(_Ethernet[iCnt]._szName, szCur);

		//dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV,                     // 하드웨어 이더넷 수신 바이트
		szQuery[0] = L'\0';
		// StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur); // wifi
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(*)\\Bytes Received/sec");
		PdhAddCounter(_PDHQuery, szQuery, NULL, &_Ethernet[iCnt]._pdh_Counter_Network_RecvBytes);

		//dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND,                     // 하드웨어 이더넷 송신 바이트
		szQuery[0] = L'\0';
		// StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur); // wifi
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(*)\\Bytes Sent/sec");
		PdhAddCounter(_PDHQuery, szQuery, NULL, &_Ethernet[iCnt]._pdh_Counter_Network_SendBytes);
	}

	//dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL = 1,                    // 하드웨어 CPU 사용률 전체
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Processor(_Total)\\%% Processor Time");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_ProcessorTime);

	//dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY,                 // 하드웨어 사용가능 메모리
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Memory\\Available MBytes");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_AvailableMBytes);

	//dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY,                  // 하드웨어 논페이지 메모리 사용량
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Memory\\Pool Nonpaged Bytes");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_NonpagedBytes);

	return true;
}

void CMonitorNetServer::mp_PACKET_CS_MONITOR_TOOL_RES_LOGIN(CNPacket * pPacket, BYTE byStatus)
{
	//------------------------------------------------------------
	// 모니터링 클라이언트(툴) 모니터링 서버로 로그인 응답
	// 로그인에 실패하면 0 보내고 끊어버림
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status					// 로그인 결과 0 / 1 
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,
	*pPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	*pPacket << byStatus;
}
