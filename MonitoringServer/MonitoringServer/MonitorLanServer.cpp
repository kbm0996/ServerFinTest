#include "MonitorLanServer.h"

CMonitorLanServer::CMonitorLanServer()//CMonitorNetServer *pNetServer)
{
	//_pNetServer = pNetServer;
	InitializeSRWLock(&_srwServerLock);

	//////////////////////////////////////////////////////////////////////////
	// LoginServer
	_bOnLoginServer = 0;
	_iCpuUsage_Login = 0;
	_iCommitMemory_Login = 0;
	_iPacketPool_Login = 0;
	_iSessionAll_Login = 0;
	_iLoginSuccessTps_Login = 0;
	// BattleServer
	_bOnGameServer = 0;		
	_iCpuUsage_Game = 0;
	_iCommitMemory_Game = 0;
	_iPacketPool_Game = 0;
	_iAuthFps_Game = 0;
	_iGameFps_Game = 0;
	_iSessionAll_Game = 0;
	_iSessionAuth_Game = 0;
	_iSessionGame_Game = 0;
	// ChatServer
	_bOnChatServer = 0;
	_iCpuUsage_Chat = 0;
	_iCommitMemory_Chat = 0;
	_iPacketPool_Chat = 0;
	_iSessionAll_Chat = 0;
	_iSessionLogin_Chat = 0;
}

CMonitorLanServer::~CMonitorLanServer()
{
}

bool CMonitorLanServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CMonitorLanServer::OnClientJoin(UINT64 SessionID)
{
	st_SERVER* pServer = new st_SERVER;
	pServer->iSessionID = SessionID;
	pServer->iUpdateTime = (int)time(NULL);
	pServer->lLastRecvTick = GetTickCount64();

	AcquireSRWLockExclusive(&_srwServerLock);
	_ServerList.push_back(pServer);
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CMonitorLanServer::OnClientLeave(UINT64 SessionID)
{
	AcquireSRWLockExclusive(&_srwServerLock);
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end(); ++Iter)
	{
		st_SERVER* pServer = *Iter;
		if (pServer->iSessionID == SessionID)
		{
			_ServerList.erase(Iter);

			switch (pServer->iServerNo)
			{
			case dfMONITOR_SERVER_TYPE_LOGIN:
				_bOnLoginServer = FALSE;
				break;
			case dfMONITOR_SERVER_TYPE_GAME:
				_bOnGameServer = FALSE;
				break;
			case dfMONITOR_SERVER_TYPE_CHAT:
				_bOnChatServer = FALSE;
				break;
			default:
				break;
			}

			delete pServer;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CMonitorLanServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		proc_PACKET_SS_MONITOR_LOGIN(SessionID, pPacket);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		proc_PACKET_SS_MONITOR_DATA_UPDATE(SessionID, pPacket);
		break;
	default:
		LOG(L"MONITOR_LANSERVER_LOG", LOG_ERROR, L"Packet Type Error : %d", wPacketType);
		break;
	}
}

void CMonitorLanServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

void CMonitorLanServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

CMonitorLanServer::st_SERVER*  CMonitorLanServer::SearchServer(UINT64 iSessionID)
{
	AcquireSRWLockExclusive(&_srwServerLock);
	st_SERVER* pServer = nullptr;
	auto Iter = _ServerList.begin();
	for (; Iter != _ServerList.end(); Iter++)
	{
		pServer = (*Iter);
		if (pServer->iSessionID == iSessionID)
			break;
	}
	if (Iter == _ServerList.end())
		pServer = nullptr;
	ReleaseSRWLockExclusive(&_srwServerLock);
	return pServer;
}

void CMonitorLanServer::ServerList_Timeout()
{
	AcquireSRWLockExclusive(&_srwServerLock);
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end();)
	{
		st_SERVER* pServer = *Iter;
		if (GetTickCount64() - pServer->lLastRecvTick > en_SERVER_TIMEOUT)
		{
			///LOG(L"LOGIN_CLIENT_LOG", LOG_DEBUG, L"%d Server Timeout", (*Iter).second->iAccountNo);

			Iter = _ServerList.erase(Iter);

			switch (pServer->iServerNo)
			{
			case dfMONITOR_SERVER_TYPE_LOGIN:
				_bOnLoginServer = FALSE;
				break;
			case dfMONITOR_SERVER_TYPE_GAME:
				_bOnGameServer = FALSE;
				break;
			case dfMONITOR_SERVER_TYPE_CHAT:
				_bOnChatServer = FALSE;
				break;
			default:
				break;
			}
			DisconnectSession(pServer->iSessionID);
			delete pServer;
			continue;
		}
		++Iter;
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CMonitorLanServer::proc_PACKET_SS_MONITOR_LOGIN(UINT64 SessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// LoginServer, GameServer , ChatServer , Agent �� ����͸� ������ �α��� ��
	//
	// 
	//	{
	//		WORD	Type
	//
	//		int		ServerNo		// ���� Ÿ�� ���� �� �������� ���� ��ȣ�� �ο��Ͽ� ���
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_MONITOR_LOGIN,
	//enum en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL
	//{
	//	dfMONITOR_SERVER_TYPE_LOGIN = 1,
	//	dfMONITOR_SERVER_TYPE_GAME = 2,
	//	dfMONITOR_SERVER_TYPE_CHAT = 3,
	//	dfMONITOR_SERVER_TYPE_AGENT = 4,

	//	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// ���� �������� (���Ӽ��� ����)
	//	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// ���� ���μ��� ��������
	//	dfMONITOR_SERVER_CONTROL_RUN = 3,			// ���� ���μ��� ���� & ����
	//};
	st_SERVER* pServer = SearchServer(SessionID);
	if (pServer == nullptr)
	{
		LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}

	*pPacket >> pServer->iServerNo;
	switch (pServer->iServerNo)
	{
	case dfMONITOR_SERVER_TYPE_LOGIN:
		if (_bOnLoginServer)
		{
			LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Already OnLoginServer");
			DisconnectSession(SessionID);
		}
		break;
	case dfMONITOR_SERVER_TYPE_GAME:
		if(_bOnGameServer)
		{
			LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Already OnGameServer");
			DisconnectSession(SessionID);
		}
		break;
	case dfMONITOR_SERVER_TYPE_CHAT:
		if(_bOnChatServer)
		{
			LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Already OnChatServer");
			DisconnectSession(SessionID);
		}
		break;
	default:
		LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Detect UnknownServer %d Login Try", pServer->iServerNo);
		DisconnectSession(SessionID);
		break;
	}
}

void CMonitorLanServer::proc_PACKET_SS_MONITOR_DATA_UPDATE(UINT64 SessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// ������ ����͸������� ������ ����
	// �� ������ �ڽ��� ����͸����� ��ġ�� 1�ʸ��� ����͸� ������ ����.
	//
	// ������ �ٿ� �� ��Ÿ ������ ����͸� �����Ͱ� ���޵��� ���ҋ��� ����Ͽ� TimeStamp �� �����Ѵ�.
	// �̴� ����͸� Ŭ���̾�Ʈ���� ���,�� ����Ѵ�.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_MONITOR_DATA_UPDATE,
	st_SERVER* pServer = SearchServer(SessionID);
	if (pServer == nullptr)
	{
		LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}

	BYTE	byDataType;
	*pPacket >> byDataType;

	switch (byDataType)
	{
	//////////////////////////////////////////////////////////////////////////
	// LoginServer
	//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // ��ġ����ŷ ���� ON
	//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // ��ġ����ŷ CPU ���� (Ŀ�� + ����)
	//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // ��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
	//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // ��ġ����ŷ ��ŶǮ ��뷮
	//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // ��ġ����ŷ ���� ����
	//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // ��ġ����ŷ ���� ���� (�α��� ���� ��)
	//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // ��ġ����ŷ �� ���� ���� �� (�ʴ�)
	case dfMONITOR_DATA_TYPE_MATCH_SERVER_ON:
		*pPacket >> _bOnLoginServer;
		break;
	case dfMONITOR_DATA_TYPE_MATCH_CPU:
		*pPacket >> _iCpuUsage_Login;
		break;
	case dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT:
		*pPacket >> _iCommitMemory_Login;
		break;
	case dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL:
		*pPacket >> _iPacketPool_Login;
		break;
	case dfMONITOR_DATA_TYPE_MATCH_SESSION:
		*pPacket >> _iSessionAll_Login;
		break;
	//case dfMONITOR_DATA_TYPE_MATCH_PLAYER:
	//	*pPacket >> _iSessionSuccess_Login;
	//	break;
	case dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS:
		*pPacket >> _iLoginSuccessTps_Login;
		break;

	//////////////////////////////////////////////////////////////////////////
	// BattleServer
	//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // ��Ʋ���� ON
	//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // ��Ʋ���� CPU ���� (Ŀ�� + ����)
	//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte
	//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // ��Ʋ���� ��ŶǮ ��뷮
	//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // ��Ʋ���� Auth ������ �ʴ� ���� ��
	//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // ��Ʋ���� Game ������ �ʴ� ���� ��
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // ��Ʋ���� ���� ������ü
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // ��Ʋ���� Auth ������ ��� �ο�
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // ��Ʋ���� Game ������ ��� �ο�
	case dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON:
		*pPacket >> _bOnGameServer;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_CPU:
		*pPacket >> _iCpuUsage_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT:
		*pPacket >> _iCommitMemory_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL:
		*pPacket >> _iPacketPool_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS:
		*pPacket >> _iAuthFps_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS:
		*pPacket >> _iGameFps_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL:
		*pPacket >> _iSessionAll_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH:
		*pPacket >> _iSessionAuth_Game;
		break;
	case dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME:
		*pPacket >> _iSessionGame_Game;
		break;
	//////////////////////////////////////////////////////////////////////////
	// ChatServer
	//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // ä�ü��� ON
	//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // ä�ü��� CPU ���� (Ŀ�� + ����)
	//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // ä�ü��� �޸� ���� Ŀ�� ��뷮 (Private) MByte
	//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // ä�ü��� ��ŶǮ ��뷮
	//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // ä�ü��� ���� ������ü
	//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // ä�ü��� �α����� ������ ��ü �ο�
	//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL,
	//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS
	case dfMONITOR_DATA_TYPE_CHAT_SERVER_ON:
		*pPacket >> _bOnChatServer;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_CPU:
		*pPacket >> _iCpuUsage_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT:
		*pPacket >> _iCommitMemory_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
		*pPacket >> _iPacketPool_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_SESSION:
		*pPacket >> _iSessionAll_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
		*pPacket >> _iSessionLogin_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
		*pPacket >> _iMsgPool_Chat;
		break;
	case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS:
		*pPacket >> _iMsgTps_Chat;
		break;

	//////////////////////////////////////////////////////////////////////////
	// Agent
	//////////////////////////////////////////////////////////////////////////
	default:
		LOG(L"MONITOR_LANSERVER_LOG", LOG_WARNG, L"Detect UnknownServer Packet");
		DisconnectSession(SessionID);
		break;
	}
	*pPacket >> pServer->iUpdateTime;
	pServer->lLastRecvTick = GetTickCount64();
}

