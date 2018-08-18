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
	// CPUUsage ����
	_CPUTime.UpdateCpuTime();
	// PDH ���� ����
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
		//����͸� ������ �������� ��������
		//
		//���� CommonProtocol.h �� �������� ���������
		//CommonProtocol.h �� ä��,�α��� �� ��Ŷ���������� ���Ƽ�
		//ȥ���� �� �� �����Ƿ� ����͸����� ��Ŷ�� ������ ���� �帳�ϴ�.
		//
		//�̸� ���� h �� �ٿ��� ��� ���ֽð�
		//���Ŀ� ä�ü��� ������� �ȳ� �� ���� ComonProtocol �� �帳�ϴ�.
		//
		//# ���� ������Ʈ ä�ú������ - MO ���� �������� ��(ä��) ������ ä�ü����� ����˴ϴ�.

		//+ ����͸� Ŭ���̾�Ʈ ���� ������ǰ ����� ���� ������ ���� �е���
		//÷�ε� MonitoringClient.cnf  ������ �����Ͽ� ���� �����Ͽ� ��� �Ͻø� �˴ϴ�.
		//
		//+ ����͸� ��������
		//������ ���Ӽ��� (���ڼ���) �� Battle �� �մϴ�.

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

// AUTH ��� ������Ʈ �̺�Ʈ ����ó����
void CBattleServer::OnAuth_Process(void)
{
}

// GAME ��� ������Ʈ �̺�Ʈ ����ó����
void CBattleServer::OnGame_Process(void)
{
}

bool CBattleServer::InitPDH()
{
	////////////////////////////////////////////////////////
	// PDH
	// ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte 
	////////////////////////////////////////////////////////
	// Create a query.
	if (PdhOpenQuery(NULL, NULL, &_PDHQuery) != ERROR_SUCCESS)
		return false;
	
	// ���� �������� ���μ����� ���ϱ�
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

	// �� ���μ����� Ŀ�� �޸𸮸� ���ϴ� ������ �ۼ�
	WCHAR szQuery[1024] = { 0, };
	StringCbPrintf(szQuery, sizeof(szQuery), L"\\Process(%s)\\Private Bytes", szProcessName); // Private Bytes :����� ���� ���� �޸� ��뷮 (���μ��� ����޸�/Ŀ�θ޸� ����)

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
	// �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//
	//		int	Version			// Major 100 + Minor 10  = 1.10
	//						// ���� �ֽ� ������		0.01 (1) - 2016.03.28
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
	// �׽�Ʈ�� ���� ��û
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
	// �׽�Ʈ�� ���� ���� (REQ �� �״�� ������)
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
	// �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status (0: ���� / 1: ���� / 2: �ű�ĳ���� ���� ��� / 3:���� �ٸ�.)
	//		INT64	AccountNo
	//	}
	//
	//  Status �� 1 �� ������ Ŭ���̾�Ʈ�� �ٷ� ������ �����ϸ�
	//  Status �� 2 �� ������ Ŭ���̾�Ʈ�� ĳ���� ���� ȭ������ ��ȯ ��.
	//
	//  ĳ���� ������ �ȵ� �������ӽ� Status 2 �� Ŭ��� ������, Status 2 �� ���� AUTH ��忡 �ӹ���.
	//
	//  Status 1 : ĳ���� ���� �ε�, ���� , GAME ��� ��ȯ �� ���� ����.
	//  Status 2 : AUTH ��� ����, REQ_CHARACTER_SELECT �� ���� �������� �Ѿ.
	//  Status 3 : ����,Ŭ���� ���� �̽���ġ 
	//
	//  en_PACKET_CS_GAME_RES_LOGIN define �� ���.
	//------------------------------------------------------------
	//en_PACKET_CS_GAME_RES_LOGIN,
	*pBuffer << (WORD)en_PACKET_CS_GAME_RES_LOGIN;

	*pBuffer << byStatus;
	*pBuffer << iAccountNo;
}