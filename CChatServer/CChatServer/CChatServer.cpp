#include "CChatServer.h"

CChatServer::CChatServer()
{
	LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG, L"CHAT_SERVER_LOG");
	if (!InitPDH())
		CRASH();

	_lMonitor_PlayerCnt = 0;
	_lMonitor_SessionMissCnt = 0;
	_lMonitor_UpdateTps = 0;

	_pLoginClient = nullptr;
	_pMonitorClient = nullptr;
	_bShutdown = false;
}

CChatServer::~CChatServer()
{
}

bool CChatServer::InitPDH()
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
		LOG(L"CHAT_SERVER_LOG", LOG_ERROR, L"GetModuleFileName failed. Error %d", GetLastError());
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

bool CChatServer::Start()
{
	int iTry = 0;

	////////////////////////////////////////////////////////////////////////////////
	// 모니터링 서버와 연결
	//
	// TODO : 해당 구간 주석처리시 모니터링서버 미연동
	////////////////////////////////////////////////////////////////////////////////
	_pMonitorClient = new CMonitorClient();
	while (!_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false))
	{
		_pMonitorClient->Stop(true);
		if (iTry > 3)
			break;

		++iTry;
	}
	/////////////////////////////////////////////////////////////////////////////////


	if (!CNetServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, CConfig::_iWorkerThreadNo, false, CConfig::_iClientMax, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
		return false;


	iTry = 0;
	////////////////////////////////////////////////////////////////////////////////
	// 로그인 서버와 연결
	//
	// TODO : 해당 구간 주석처리시 로그인서버 미연동
	////////////////////////////////////////////////////////////////////////////////
	_pLoginClient = new CLoginClient(this);
	while (!_pLoginClient->Start(CConfig::_szLoginLanServerIP, CConfig::_iLoginLanServerPort, 1, false))
	{
		_pLoginClient->Stop(true);
		if (iTry > 3)
			break;

		++iTry;
	}
	/////////////////////////////////////////////////////////////////////////////////

	_hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hUpdateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, NULL);
}

void CChatServer::Stop()
{
	_bShutdown = true;

	SetEvent(_hUpdateEvent);
	WaitForSingleObject(_hUpdateThread, INFINITE);
	CloseHandle(_hUpdateThread);
	CloseHandle(_hUpdateEvent);

	for (auto iter = _PlayerMap.begin(); iter != _PlayerMap.end();)
	{
		_PlayerPool.Free(iter->second);
		_PlayerMap.erase(iter++);
	}
	if (_pMonitorClient != nullptr)
		_pMonitorClient->Stop();

	if (_pLoginClient != nullptr)
		_pLoginClient->Stop();

	CNetServer::Stop();

	if (_pLoginClient != nullptr)
	{
		delete _pLoginClient;
		_pLoginClient = nullptr;
	}
	if (_pMonitorClient != nullptr)
	{
		delete _pMonitorClient;
		_pMonitorClient = nullptr;
	}
}

void CChatServer::Monitoring()
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

	wprintf(L"===========================================\n");
	wprintf(L" Chat Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - MessagePool		: %d/%d \n", _MessagePool.GetUseSize(), _MessagePool.GetAllocSize());
	wprintf(L" - MessageTPS		: %d \n", _lMonitor_UpdateTps);		// UpdateThread 큐 남은 개수
	wprintf(L"\n");
	wprintf(L" - PlayerPool		: %d \n", _PlayerPool.GetAllocSize());		// Player 구조체 할당량
	wprintf(L" - PlayerCount		: %lld \n", _lMonitor_PlayerCnt);	// Contents 파트 Player 개수
	wprintf(L"\n");
	wprintf(L" - SessionKey Miss	: %lld \n", _lMonitor_SessionMissCnt);
	if (_pLoginClient != nullptr)
		wprintf(L" - LoginMap Size	: %d \n", _pLoginClient->LoginMap_Size());

	CNetServer::PrintState(true, false, true, true);

	wprintf(L"\n");
	wprintf(L" - CPU Usage		: %d\n", (int)_CPUTime.ProcessTotal());
	wprintf(L" - Commit Memory	: %dM\n", (int)dPrivateMemory);

	if (_pLoginClient != nullptr)
	{
		if (!_pLoginClient->_bConnection)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"LoginServer Reconnect");
			_pLoginClient->Stop(true);
			_pLoginClient->Start(CConfig::_szLoginLanServerIP, CConfig::_iLoginLanServerPort, 1, false);
		}
		else
		{
			// 접속만 하고 채팅 서버로 접속하지 않는 유저 제거
			_pLoginClient->LoginMap_Timeout();
		}
	}

	if (_pMonitorClient != nullptr)
	{
		if (!_pMonitorClient->_bConnection)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"MonitorServer Reconnect");
			_pMonitorClient->Stop(true);
			_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false);
		}
		else
		{
			//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // 채팅서버 ON
			//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // 채팅서버 CPU 사용률 (커널 + 유저)
			//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
			//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // 채팅서버 패킷풀 사용량
			//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // 채팅서버 접속 세션전체
			//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // 채팅서버 로그인을 성공한 전체 인원
			//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL,
			//dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS
			int iUsePacketCnt = CNPacket::GetUseSize();
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_ON, TRUE);
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_CPU, _CPUTime.ProcessTotal());
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT, Counter.doubleValue / 1024 / 1024);
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, iUsePacketCnt);

			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SESSION, _lConnectCnt);
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, _lMonitor_PlayerCnt);
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, _MessagePool.GetUseSize());
			_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_TPS, _lMonitor_UpdateTps);
		}
	}

	_lMonitor_UpdateTps = 0;
}

bool CChatServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CChatServer::OnClientJoin(UINT64 SessionID)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread에서 Free
	pMessage->wType = en_MSG_JOIN;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = nullptr;

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}

void CChatServer::OnClientLeave(UINT64 SessionID)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread에서 Free
	pMessage->wType = en_MSG_LEAVE;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = nullptr;

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}

void CChatServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread에서 Free
	pMessage->wType = en_MSG_PACKET;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = pPacket;

	pPacket->AddRef(); // proc_MSG_PACKET()에서 Free

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}

void CChatServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

void CChatServer::OnError(int iErrCode, WCHAR * wszErr)
{
}


unsigned int CChatServer::UpdateThread(LPVOID pCChatServer)
{
	return ((CChatServer*)pCChatServer)->UpdateThread_Process();
}

unsigned int CChatServer::UpdateThread_Process()
{
	ULONGLONG lHeartBeatTick = GetTickCount64();
	while (!_bShutdown)
	{
		WaitForSingleObject(_hUpdateEvent, INFINITE);

		st_MESSAGE *pMessage = nullptr;
		while (_MessageQ.Dequeue(pMessage))
		{
			if (pMessage == nullptr)
				break;

			++_lMonitor_UpdateTps;

			switch (pMessage->wType)
			{
			case en_MSG_JOIN:
				proc_MSG_JOIN(pMessage);
				break;
			case en_MSG_LEAVE:
				proc_MSG_LEAVE(pMessage);
				break;
			case en_MSG_PACKET:
				proc_MSG_PACKET(pMessage);
				break;
			case en_MSG_HEARTBEAT:
				break;
			default:

				break;
			}
			if(pMessage->pPacket != nullptr)
				pMessage->pPacket->Free();
			_MessagePool.Free(pMessage);
		}
	}
	return 0;
}

void CChatServer::proc_MSG_JOIN(st_MESSAGE * pMessage)
{
	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	pPlayer->iSessionID = pMessage->iSessionID;
	pPlayer->iAccountNo = 0;
	pPlayer->shSectorX = -1;
	pPlayer->shSectorY = -1;

	pPlayer->lLastRecvTick = GetTickCount64();

	// PlayerMap에 추가
	_PlayerMap.insert(pair<UINT64, st_PLAYER*>(pPlayer->iSessionID, pPlayer));
}

void CChatServer::proc_MSG_LEAVE(st_MESSAGE * pMessage)
{
	st_PLAYER* pPlayer = SearchPlayer(pMessage->iSessionID);
	if (pPlayer == nullptr)
		return;

	if (pPlayer->iAccountNo != 0)
		--_lMonitor_PlayerCnt;

	// 소속된 섹터에서 지우기
	LeaveSector(pPlayer);

	// PlayerMap에서 지우기
	_PlayerMap.erase(pMessage->iSessionID);
	_PlayerPool.Free(pPlayer);
}

void CChatServer::proc_MSG_PACKET(st_MESSAGE * pMessage)
{
	CNPacket *pPacket = pMessage->pPacket;
	if (pPacket == nullptr)
		CRASH();
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		proc_PACKET_CS_CHAT_REQ_LOGIN(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		proc_PACKET_CS_CHAT_REQ_SECTOR_MOVE(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		proc_PACKET_CS_CHAT_REQ_MESSAGE(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		proc_PACKET_CS_CHAT_REQ_HEARTBEAT(pMessage->iSessionID, pPacket);
		break;
	default:
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Unknown PacketType %d", wPacketType);
		DisconnectSession(pMessage->iSessionID);
		break;
	}
}


bool CChatServer::GetSectorAround(short shSectorX, short shSectorY, st_SECTOR_AROUND * pSectorAround)
{
	if (shSectorX == -1 || shSectorY == -1)
		return false;

	--shSectorX;
	--shSectorY;

	pSectorAround->iCount = 0;
	for (int iCntY = 0; iCntY < 3; ++iCntY)
	{
		if (shSectorY + iCntY < 0 || shSectorY + iCntY >= en_SECTOR_MAX_Y)
			continue;
		for (int iCntX = 0; iCntX < 3; ++iCntX)
		{
			if (shSectorX + iCntX < 0 || shSectorX + iCntX >= en_SECTOR_MAX_X)
				continue;

			pSectorAround->Around[pSectorAround->iCount].shSectorX = shSectorX + iCntX;
			pSectorAround->Around[pSectorAround->iCount].shSectorY = shSectorY + iCntY;
			++pSectorAround->iCount;
		}
	}
	return true;
}

bool CChatServer::SetSector(st_PLAYER * pPlayer, short shSectorX, short shSectorY)
{
	LeaveSector(pPlayer);
	if (!EnterSector(pPlayer, shSectorX, shSectorY))
		return false;

	return true;
}

bool CChatServer::EnterSector(st_PLAYER * pPlayer, short shSectorX, short shSectorY)
{
	// 섹터 범위 초과
	if (shSectorX >= en_SECTOR_MAX_X || shSectorY >= en_SECTOR_MAX_Y || shSectorX < 0 || shSectorY < 0)
		return false;

	_Sector[shSectorY][shSectorX].push_back(pPlayer);
	pPlayer->shSectorX = shSectorX;
	pPlayer->shSectorY = shSectorY;

	return true;
}

bool CChatServer::LeaveSector(st_PLAYER * pPlayer)
{
	// 섹터 미지정
	if (pPlayer->shSectorX == -1 || pPlayer->shSectorY == -1)
		return false;

	// 비어있는 리스트
	if (_Sector[pPlayer->shSectorY][pPlayer->shSectorX].size() == 0)
		return false;

	_Sector[pPlayer->shSectorY][pPlayer->shSectorX].remove(pPlayer);

	pPlayer->shSectorX = -1;
	pPlayer->shSectorY = -1;

	return true;
}

void CChatServer::SendPacket_Around(st_PLAYER * pPlayer, CNPacket * pPacket, bool bSendMe)
{
	st_SECTOR_AROUND	stSectorAround;
	if (!GetSectorAround(pPlayer->shSectorX, pPlayer->shSectorY, &stSectorAround))
	{
		LOG(L"SECTOR_LOG", LOG_ERROR, L"Sector Around Find Error [X:%d / Y:%d]", pPlayer->shSectorX, pPlayer->shSectorY);
		return;
	}

	list<st_PLAYER*> *pSectorList;
	list<st_PLAYER*>::iterator Iter_List;
	for (int iCnt = 0; iCnt < stSectorAround.iCount; iCnt++)
	{
		pSectorList = &_Sector[stSectorAround.Around[iCnt].shSectorY][stSectorAround.Around[iCnt].shSectorX];
		for (Iter_List = pSectorList->begin(); Iter_List != pSectorList->end(); ++Iter_List)
		{
			if (bSendMe == FALSE)
			{
				if ((*Iter_List)->iSessionID == pPlayer->iSessionID)
					continue;
			}
			SendPacket((*Iter_List)->iSessionID, pPacket);
		}
	}
}

CChatServer::st_PLAYER * CChatServer::SearchPlayer(UINT64 iSessionID)
{
	map<UINT64, st_PLAYER*>::iterator iter = _PlayerMap.find(iSessionID);
	if (iter == _PlayerMap.end())
		return nullptr;
	return iter->second;
}

void CChatServer::proc_PACKET_CS_CHAT_REQ_LOGIN(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_REQ_LOGIN,
	BYTE byResult = 1;
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_ERROR, L"Player Not Find");
		DisconnectSession(iSessionID);
		return;
	}

	*pPacket >> pPlayer->iAccountNo;
	pPacket->GetData((char*)pPlayer->szID, sizeof(pPlayer->szID));
	pPacket->GetData((char*)pPlayer->szNickname, sizeof(pPlayer->szNickname));
	pPacket->GetData((char*)pPlayer->szSessionKey, sizeof(pPlayer->szSessionKey));
	pPlayer->lLastRecvTick = GetTickCount64();

	// 로그인 성공시 로그인 리스트에서 제거
	if (_pLoginClient != nullptr)
	{
		if (_pLoginClient->_bConnection)
		{
			if (!_pLoginClient->LoginMap_Remove(pPlayer->iAccountNo, pPlayer->szSessionKey))
				byResult = 0;
		}
	}

	++_lMonitor_PlayerCnt;

	CNPacket* pSendPacket = CNPacket::Alloc();
	mp_PACKET_CS_CHAT_RES_LOGIN(pSendPacket, byResult, pPlayer->iAccountNo);

	if(byResult == 1)
		SendPacket(iSessionID, pSendPacket);
	else
		SendPacket_Disconnect(iSessionID, pSendPacket);

	pSendPacket->Free();

}

void CChatServer::proc_PACKET_CS_CHAT_REQ_SECTOR_MOVE(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,
	st_PLAYER* pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}
	INT64	iAccountNo;
	short	shSectorX;
	short	shSectorY;
	*pPacket >> iAccountNo;
	*pPacket >> shSectorX;
	*pPacket >> shSectorY;

	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Packet AccountNo Error Player : %lld != %lld", pPlayer->iAccountNo, iAccountNo);
		DisconnectSession(iSessionID);
		return;
	}
	if (!SetSector(pPlayer, shSectorX, shSectorY))
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"SetSector Out of Range");
		DisconnectSession(iSessionID);
		return;
	}

	// 최종 메시지 시간 갱신
	pPlayer->lLastRecvTick = GetTickCount64();

	CNPacket* pSendPacket = CNPacket::Alloc();
	mp_PACKET_CS_CHAT_RES_SECTOR_MOVE(pSendPacket, iAccountNo, pPlayer->shSectorX, pPlayer->shSectorY);
	SendPacket(iSessionID, pSendPacket);
	pSendPacket->Free();

}

void CChatServer::proc_PACKET_CS_CHAT_REQ_MESSAGE(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_REQ_MESSAGE,
	st_PLAYER* pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}

	INT64	iAccountNo;
	WORD	wMessageLen;
	WCHAR	szMessage[256];
	ZeroMemory(szMessage, sizeof(szMessage));

	*pPacket >> iAccountNo;
	*pPacket >> wMessageLen;
	pPacket->GetData((char*)szMessage, wMessageLen);

	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_ERROR, L"Packet AccountNo Error Player : %lld != %lld", pPlayer->iAccountNo, iAccountNo);
		DisconnectSession(iSessionID);
		return;
	}

	pPlayer->lLastRecvTick = GetTickCount64();

	CNPacket* pSendPacket = CNPacket::Alloc();
	mp_PACKET_CS_CHAT_RES_MESSAGE(pSendPacket, pPlayer->iAccountNo, pPlayer->szID, pPlayer->szNickname, wMessageLen, szMessage);
	SendPacket_Around(pPlayer, pSendPacket, true);
	pSendPacket->Free();
}

void CChatServer::proc_PACKET_CS_CHAT_REQ_HEARTBEAT(UINT64 iSessionID, CNPacket * pPacket)
{
	st_PLAYER* pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}

	pPlayer->lLastRecvTick = GetTickCount64();
}

void CChatServer::mp_PACKET_CS_CHAT_RES_LOGIN(CNPacket * pBuffer, BYTE byStatus, INT64 iAccountNo)
{
	//------------------------------------------------------------
	// 채팅서버 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:실패	1:성공
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_RES_LOGIN,
	*pBuffer << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;

	*pBuffer << byStatus;
	*pBuffer << iAccountNo;
}

void CChatServer::mp_PACKET_CS_CHAT_RES_SECTOR_MOVE(CNPacket * pBuffer, INT64 iAccountNo, WORD wSectorX, WORD wSectorY)
{
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 결과
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_RES_SECTOR_MOVE,
	*pBuffer << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	*pBuffer << iAccountNo;
	*pBuffer << wSectorX;
	*pBuffer << wSectorY;
}

void CChatServer::mp_PACKET_CS_CHAT_RES_MESSAGE(CNPacket * pBuffer, INT64 iAccountNo, WCHAR * szID, WCHAR * szNickname, WORD wMessageLen, WCHAR * szMessage)
{
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null 포함
	//		WCHAR	Nickname[20]				// null 포함
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_CS_CHAT_RES_MESSAGE,
	*pBuffer << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;

	*pBuffer << iAccountNo;
	pBuffer->PutData((char*)szID, sizeof(WCHAR) * 20);
	pBuffer->PutData((char*)szNickname, sizeof(WCHAR) * 20);

	*pBuffer << wMessageLen;
	pBuffer->PutData((char*)szMessage, wMessageLen);
}