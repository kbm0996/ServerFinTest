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
	// LoginServer, GameServer , ChatServer , Agent 가 모니터링 서버에 로그인 함
	//
	// 
	//	{
	//		WORD	Type
	//
	//		int		ServerNo		// 서버 타입 없이 각 서버마다 고유 번호를 부여하여 사용
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

	//	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// 서버 정상종료 (게임서버 전용)
	//	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// 서버 프로세스 강제종료
	//	dfMONITOR_SERVER_CONTROL_RUN = 3,			// 서버 프로세스 생성 & 실행
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
	// 서버가 모니터링서버로 데이터 전송
	// 각 서버는 자신이 모니터링중인 수치를 1초마다 모니터링 서버로 전송.
	//
	// 서버의 다운 및 기타 이유로 모니터링 데이터가 전달되지 못할떄를 대비하여 TimeStamp 를 전달한다.
	// 이는 모니터링 클라이언트에서 계산,비교 사용한다.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
	//		int		DataValue				// 해당 데이터 수치.
	//		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
	//										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
	//										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
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
	//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // 매치메이킹 서버 ON
	//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // 매치메이킹 CPU 사용률 (커널 + 유저)
	//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // 매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
	//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // 매치메이킹 패킷풀 사용량
	//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // 매치메이킹 접속 세션
	//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // 매치메이킹 접속 유저 (로그인 성공 후)
	//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // 매치메이킹 방 배정 성공 수 (초당)
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
	//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // 배틀서버 ON
	//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // 배틀서버 CPU 사용률 (커널 + 유저)
	//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
	//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // 배틀서버 패킷풀 사용량
	//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // 배틀서버 Auth 스레드 초당 루프 수
	//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // 배틀서버 Game 스레드 초당 루프 수
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // 배틀서버 접속 세션전체
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // 배틀서버 Auth 스레드 모드 인원
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // 배틀서버 Game 스레드 모드 인원
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
	//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // 채팅서버 ON
	//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // 채팅서버 CPU 사용률 (커널 + 유저)
	//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
	//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // 채팅서버 패킷풀 사용량
	//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // 채팅서버 접속 세션전체
	//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // 채팅서버 로그인을 성공한 전체 인원
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

