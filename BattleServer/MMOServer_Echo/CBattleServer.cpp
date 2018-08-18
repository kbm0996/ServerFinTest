#include "CBattleServer.h"

using namespace mylib;

CBattleServer::CBattleServer(int iMaxSession) : CMMOServer(iMaxSession)
{
	LOG_SET(LOG_CONSOLE|LOG_FILE, LOG_DEBUG, L"MMO_SERVER_LOG");

	if (!InitPDH())
		CRASH();

	_pPlayer = new CPlayer[iMaxSession];
	for (int i = 0; i < iMaxSession; ++i)
	{
		SetSessionArray(i, &_pPlayer[i]);
		_pPlayer[i]._pServer = this;
	}

	_pMonitorClient = nullptr;
	_bShutdown = false;
}

CBattleServer::~CBattleServer()
{
	if (_pMonitorClient != nullptr)
		delete _pMonitorClient;
}

bool CBattleServer::Start()
{
	int iMonitorTry = 0;
	_pMonitorClient = new CMonitorClient();
	while (!_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false))
	{
		_pMonitorClient->Stop(true);
		Sleep(1000);

		if (iMonitorTry > 3)
			break;

		++iMonitorTry;
	}

	if (!CMMOServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, CConfig::_iWorkerThreadNo, true, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
	{
		if (_pMonitorClient != nullptr)
			_pMonitorClient->Stop();
		return false;
	}
	return true;
}

bool CBattleServer::Stop()
{
	_bShutdown = true;

	if (_pMonitorClient != nullptr)
	{
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, FALSE);
		_pMonitorClient->Stop();
	}

	CMMOServer::Stop();

	_bShutdown = false;
	return true;
}

void CBattleServer::Monitoring()
{
	// CPUUsage 갱신
	_CPUTime.UpdateCpuTime();
	// PDH 쿼리 갱신
	// Print counter values
	PdhCollectQueryData(_PDHQuery);
	// Compute a displayable value for the counter.
	PDH_FMT_COUNTERVALUE Counter;
	double dPrivateMemory = 0;
	if (PdhGetFormattedCounterValue(_pdh_Counter_PrivateBytes, PDH_FMT_DOUBLE, NULL, &Counter) == ERROR_SUCCESS)
	{
		dPrivateMemory = Counter.doubleValue / 1024 / 1024;
	}

	int iUsePacketCnt = CNPacket::GetUseSize();
	int iAllocPacketCnt = CNPacket::GetAllocSize();
	wprintf(L"===========================================\n");
	wprintf(L" Echo Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - AcceptSocket		: %ld \n", _Monitor_AcceptSocket);
	wprintf(L" - ALL Mode		: %d\n", _Monitor_SessionAllMode);
	wprintf(L" - Auth Mode		: %d\n", _Monitor_SessionAuthMode);
	wprintf(L" - Game Mode		: %d\n", _Monitor_SessionGameMode);
	wprintf(L"\n");
	wprintf(L" - Auth Update FPS	: %d\n", _Monitor_Counter_AuthUpdate);
	wprintf(L" - Game Update FPS	: %d\n", _Monitor_Counter_GameUpdate);
	wprintf(L" - Packet Proc TPS	: %d\n", _Monitor_Counter_PacketProc);
	wprintf(L" - Packet Send TPS	: %d\n", _Monitor_Counter_PacketSend);
	wprintf(L"\n");
	wprintf(L" - Packet Pool		: %d/%d\n", iUsePacketCnt, iAllocPacketCnt);
	wprintf(L" - AcceptSocketQueue	: %d/%d\n", _AcceptSocketPool.GetUseSize(), _AcceptSocketPool.GetAllocSize());
	wprintf(L" - AcceptSocketPool	: %d/%d\n", _AcceptSocketQueue.GetUseSize(), _AcceptSocketQueue.GetAllocSize());
	wprintf(L"\n");
	wprintf(L" - CPU Usage		: %d\n", (int)_CPUTime.ProcessTotal());
	wprintf(L" - Commit Memory	: %dM\n", (int)dPrivateMemory);
	
	if (_pMonitorClient == nullptr)
	{
		_Monitor_Counter_AuthUpdate = _Monitor_Counter_GameUpdate = _Monitor_Counter_PacketProc = _Monitor_Counter_PacketSend = 0;
		return;
	}
	if (!_pMonitorClient->_bConnection)
	{
		LOG(L"GAME_SERVER_LOG", LOG_ERROR, L"MonitorServer Reconnect");
		_pMonitorClient->Stop(true);
		_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false);
	}
	else
	{
		//모니터링 서버와 연동관련 프로토콜
		//
		//본래 CommonProtocol.h 에 통합으로 들어있으나
		//CommonProtocol.h 의 채팅,로그인 등 패킷변동사항이 많아서
		//혼동이 올 수 있으므로 모니터링관련 패킷만 별도로 빼서 드립니다.
		//
		//이를 따로 h 에 붙여서 사용 해주시고
		//차후에 채팅서버 변경사항 안내 후 최종 ComonProtocol 을 드립니다.
		//
		//# 최종 프로젝트 채팅변경사항 - MO 서버 구성으로 방(채널) 개념의 채팅서버로 변경됩니다.

		//+ 모니터링 클라이언트 각자 개인제품 사용을 권장 하지만 없는 분들은
		//첨부된 MonitoringClient.cnf  설정을 참고하여 각자 수정하여 사용 하시면 됩니다.
		//
		//+ 모니터링 프로토콜
		//지금의 게임서버 (에코서버) 는 Battle 로 합니다.

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

		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, TRUE);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_CPU, _CPUTime.ProcessTotal());
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, Counter.doubleValue / 1024 /1024);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, iUsePacketCnt);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, _Monitor_Counter_AuthUpdate);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, _Monitor_Counter_GameUpdate);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, _Monitor_SessionAllMode);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, _Monitor_SessionAuthMode);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, _Monitor_SessionGameMode);
	}

	_Monitor_Counter_AuthUpdate = _Monitor_Counter_GameUpdate = _Monitor_Counter_PacketProc = _Monitor_Counter_PacketSend = 0;
}

// AUTH 모드 업데이트 이벤트 로직처리부
void CBattleServer::OnAuth_Process(void)
{
}

// GAME 모드 업데이트 이벤트 로직처리부
void CBattleServer::OnGame_Process(void)
{
}

bool CBattleServer::InitPDH()
{
	////////////////////////////////////////////////////////
	// PDH
	// 배틀서버 메모리 유저 커밋 사용량 (Private) MByte 
	////////////////////////////////////////////////////////
	// Create a query.
	if (PdhOpenQuery(NULL, NULL, &_PDHQuery) != ERROR_SUCCESS)
		return false;
	
	// 현재 실행중인 프로세스명 구하기
	WCHAR szPath[MAX_PATH + 1] = { 0, };
	WCHAR* szProcessName;
	int iPathLen;
	int iProcessNameLen = 0;
	WCHAR* pStartPos;
	WCHAR* pEndPos;

	iPathLen = GetModuleFileName(NULL, szPath, MAX_PATH);
	if (iPathLen == 0)
	{
		LOG(L"GAME_SERVER_LOG", LOG_ERROR, L"GetModuleFileName failed. Error %d", GetLastError());
		return false;
	}
	pStartPos = wcsrchr(szPath, '\\');
	pEndPos = wcsrchr(szPath, '.');
	iProcessNameLen = pEndPos - pStartPos;

	szProcessName = (WCHAR*)malloc(iProcessNameLen * sizeof(WCHAR));
	memset(szProcessName, 0, iProcessNameLen * sizeof(WCHAR));
	memcpy(szProcessName, pStartPos + 1, iProcessNameLen * sizeof(WCHAR));
	szProcessName[iProcessNameLen - 1] = '\0';

	free(szProcessName);

	// 현 프로세스의 커밋 메모리를 구하는 쿼리문 작성
	WCHAR szQuery[1024] = { 0, };
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Process(%s)\\Private Bytes", szProcessName); // Private Bytes :사용한 실제 유저 메모리 사용량 (프로세스 공용메모리/커널메모리 제외)

	if (PdhAddCounter(_PDHQuery, szQuery, NULL, &_pdh_Counter_PrivateBytes) != ERROR_SUCCESS)
		return false;

	return true;
}


CPlayer::CPlayer()
{
	_pServer = nullptr;
	_iAccountNo = -1;
}

CPlayer::~CPlayer()
{
}

void CPlayer::OnAuth_ClientJoin(void)
{
}

void CPlayer::OnAuth_ClientLeave(void)
{
}

void CPlayer::OnAuth_Packet(CNPacket * pPacket)
{
	WORD wType;
	*pPacket >> wType;

	switch (wType)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:
		OnAuth_PacketProc_REQ_LOGIN(pPacket);
		break;
	default:
		break;
	}
}

void CPlayer::OnGame_ClientJoin(void)
{
}

void CPlayer::OnGame_ClientLeave(void)
{
}

void CPlayer::OnGame_Packet(CNPacket * pPacket)
{
	WORD wType;
	*pPacket >> wType;

	switch (wType)
	{
	case en_PACKET_CS_GAME_REQ_ECHO:
		OnGame_PacketProc_REQ_ECHO(pPacket);
		break;
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		OnGame_PacketProc_REQ_HEARTBEAT();
		break;
	default:
		break;
	}
}

void CPlayer::OnGame_ClientRelease(void)
{
}

void CPlayer::OnTimeout(void)
{
}

void CPlayer::OnAuth_PacketProc_REQ_LOGIN(CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//
	//		int	Version			// Major 100 + Minor 10  = 1.10
	//						// 현재 최신 버전은		0.01 (1) - 2016.03.28
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_GAME_REQ_LOGIN,
	BYTE byResult = 1;
	int iVersion;

	*pPacket >> _iAccountNo;
	pPacket->GetData(_szSessionKey, 64);
	*pPacket >> iVersion;
	///printf("%d %d %s %d\n", wType, _AccountNo, _szSessionKey, iVersion);

	CNPacket* pSendPacket = CNPacket::Alloc();
	mpResLogin(pSendPacket, byResult, _iAccountNo);
	SendPacket(pSendPacket);
	//SendPacket_Disconnect(pSendPacket);
	pSendPacket->Free();

	SetMode_GAME();
}

void CPlayer::OnGame_PacketProc_REQ_ECHO(CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 테스트용 에코 요청
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------	
	//en_PACKET_CS_GAME_REQ_ECHO = 5000,
	INT64 iAccountNo;
	LONGLONG lSendTick;
	*pPacket >> iAccountNo;
	*pPacket >> lSendTick;

	if (_iAccountNo != iAccountNo)
	{
		LOG(L"GAME_SERVER_LOG", LOG_ERROR, L"AccountNo Not Matching %d : P%d", _iAccountNo, iAccountNo);
		Disconnect();
	}

	//------------------------------------------------------------
	// 테스트용 에코 응답 (REQ 를 그대로 돌려줌)
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_GAME_RES_ECHO,
	CNPacket* pSendPacket = CNPacket::Alloc();

	*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_ECHO;
	*pSendPacket << _iAccountNo;
	*pSendPacket << lSendTick;

	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CPlayer::OnGame_PacketProc_REQ_HEARTBEAT()
{
	_lLastRecvTick = GetTickCount64();
}



void CPlayer::mpResLogin(CNPacket * pBuffer, BYTE byStatus, INT64 iAccountNo)
{
	//------------------------------------------------------------
	// 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status (0: 실패 / 1: 성공 / 2: 신규캐릭터 선택 모드 / 3:버전 다름.)
	//		INT64	AccountNo
	//	}
	//
	//  Status 가 1 로 들어오면 클라이언트는 바로 게임을 시작하며
	//  Status 가 2 로 들어오면 클라이언트는 캐릭터 선택 화면으로 전환 됨.
	//
	//  캐릭터 선택이 안된 최초접속시 Status 2 를 클라로 보내며, Status 2 의 경우는 AUTH 모드에 머무름.
	//
	//  Status 1 : 캐릭터 정보 로드, 셋팅 , GAME 모드 전환 후 게임 시작.
	//  Status 2 : AUTH 모드 유지, REQ_CHARACTER_SELECT 가 오면 다음으로 넘어감.
	//  Status 3 : 서버,클라의 버전 미스매치 
	//
	//  en_PACKET_CS_GAME_RES_LOGIN define 값 사용.
	//------------------------------------------------------------
	//en_PACKET_CS_GAME_RES_LOGIN,
	*pBuffer << (WORD)en_PACKET_CS_GAME_RES_LOGIN;

	*pBuffer << byStatus;
	*pBuffer << iAccountNo;
}