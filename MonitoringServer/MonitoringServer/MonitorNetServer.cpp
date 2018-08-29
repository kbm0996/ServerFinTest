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
	// ����͸� Ŭ���̾�Ʈ(��) �� ����͸� ������ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		char	LoginSessionKey[32]		// �α��� ���� Ű. (�̴� ����͸� ������ ���������� ����)
	//										// �� ����͸� ���� ���� Ű�� ������ ���;� ��
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
	// ����͸� Ŭ���̾�Ʈ(��) ����͸� ������ �α��� ����
	// �α��ο� �����ϸ� 0 ������ �������
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status					// �α��� ��� 0 / 1 
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
	//dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL = 1,                    // �ϵ���� CPU ���� ��ü
	//dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY,                 // �ϵ���� ��밡�� �޸�
	//dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV,                     // �ϵ���� �̴��� ���� ����Ʈ
	//dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND,                     // �ϵ���� �̴��� �۽� ����Ʈ
	//dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY,                  // �ϵ���� �������� �޸� ��뷮
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_MASTER_SERVER_ON, TRUE);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL, _pdh_Value_ProcessorTime);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY, _pdh_Value_AvailableMBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, _pdh_Value_Network_RecvBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND, _pdh_Value_Network_SendBytes);
	SendData(dfMONITOR_SERVER_TYPE_AGENT, dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY, _pdh_Value_NonpagedBytes);

	if (_pLanServer->_bOnLoginServer)
	{
		//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // ��ġ����ŷ ���� ON
		//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // ��ġ����ŷ CPU ���� (Ŀ�� + ����)
		//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // ��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
		//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // ��ġ����ŷ ��ŶǮ ��뷮
		//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // ��ġ����ŷ ���� ����
		//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // ��ġ����ŷ ���� ���� (�α��� ���� ��)
		//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // ��ġ����ŷ �� ���� ���� �� (�ʴ�)
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
		//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // ��Ʋ���� ON
		//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // ��Ʋ���� CPU ���� (Ŀ�� + ����)
		//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte
		//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // ��Ʋ���� ��ŶǮ ��뷮
		//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // ��Ʋ���� Auth ������ �ʴ� ���� ��
		//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // ��Ʋ���� Game ������ �ʴ� ���� ��
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // ��Ʋ���� ���� ������ü
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // ��Ʋ���� Auth ������ ��� �ο�
		//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // ��Ʋ���� Game ������ ��� �ο�
		//dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // ��Ʋ���� ���� ��
		//dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // ��Ʋ���� �÷��̹� ��
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
		//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // ä�ü��� ON
		//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // ä�ü��� CPU ���� (Ŀ�� + ����)
		//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // ä�ü��� �޸� ���� Ŀ�� ��뷮 (Private) MByte
		//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // ä�ü��� ��ŶǮ ��뷮
		//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // ä�ü��� ���� ������ü
		//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // ä�ü��� �α����� ������ ��ü �ο�
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
	// CPUUsage ����
	_CPUTime.UpdateCpuTime();

	/////////////////////////////////////////////////////////////////////////////////
	// PDH
	/////////////////////////////////////////////////////////////////////////////////
	// PDH ���� ����
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
	// ����͸� ������ ����͸� Ŭ���̾�Ʈ(��) ���� ����͸� ������ ����
	//
	// ����͸� ������ ��� ����͸� Ŭ���̾�Ʈ���� ��� �����͸� �ѷ��ش�.
	//
	// �����͸� �����ϱ� ���ؼ��� �ʴ����� ��� �����͸� ��� 30~40���� ����͸� �����͸� �ϳ��� ��Ŷ���� ����°�
	// ������  �������� ������ ������ �����Ƿ� �׳� ������ ����͸� �����͸� ���������� ����ó�� �Ѵ�.
	//
	//	{
	//		WORD	Type
	//		
	//		BYTE	ServerNo				// ���� No
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
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
	// ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte 
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
	// ������ ���� ���ϱ� : OutBuffer ���ڵ��� ��� NULL��
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

	// szInterfaces ���� ���ڿ� ������ �����鼭 , �̸��� ����޴´�.
	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_Ethernet[iCnt]._bUse = true;
		_Ethernet[iCnt]._szName[0] = L'\0';

		wcscpy_s(_Ethernet[iCnt]._szName, szCur);

		//dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV,                     // �ϵ���� �̴��� ���� ����Ʈ
		szQuery[0] = L'\0';
		// StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur); // wifi
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(*)\\Bytes Received/sec");
		PdhAddCounter(_PDHQuery, szQuery, NULL, &_Ethernet[iCnt]._pdh_Counter_Network_RecvBytes);

		//dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND,                     // �ϵ���� �̴��� �۽� ����Ʈ
		szQuery[0] = L'\0';
		// StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur); // wifi
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(*)\\Bytes Sent/sec");
		PdhAddCounter(_PDHQuery, szQuery, NULL, &_Ethernet[iCnt]._pdh_Counter_Network_SendBytes);
	}

	//dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL = 1,                    // �ϵ���� CPU ���� ��ü
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Processor(_Total)\\%% Processor Time");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_ProcessorTime);

	//dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY,                 // �ϵ���� ��밡�� �޸�
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Memory\\Available MBytes");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_AvailableMBytes);

	//dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY,                  // �ϵ���� �������� �޸� ��뷮
	szQuery[0] = L'\0';
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Memory\\Pool Nonpaged Bytes");
	PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_NonpagedBytes);

	return true;
}

void CMonitorNetServer::mp_PACKET_CS_MONITOR_TOOL_RES_LOGIN(CNPacket * pPacket, BYTE byStatus)
{
	//------------------------------------------------------------
	// ����͸� Ŭ���̾�Ʈ(��) ����͸� ������ �α��� ����
	// �α��ο� �����ϸ� 0 ������ �������
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status					// �α��� ��� 0 / 1 
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,
	*pPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	*pPacket << byStatus;
}
