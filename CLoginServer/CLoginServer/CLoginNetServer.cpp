#include "CLoginNetServer.h"
#include <conio.h>
#include "CConfig.h"

extern CConfig* pConfig;

CLoginServer::CLoginServer()
{
	LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG, L"LOGIN_NET_SERVER_LOG");

	if (!InitPDH())
		CRASH();

	// Thread
	_hUpdateThread;
	_hUpdateEvent;
	_bShutdown = false;
	_bControlMode = false;

	// Monitoring
	_lLoginRequestCnt = 0;
	_lLoginSuccessTps = 0;
	_lLoginSuccessTime_Min = 9999999;
	_lLoginSuccessTime_Max = 0;
	_lLoginSuccessTime_Cnt = 0;

	// Server to Server
	_bConnectChatServer = false;

	InitializeSRWLock(&_srwPlayerLock);

	_pLoginLanServer = nullptr;
	_pDBAccount = nullptr;
	_pMonitorClient = nullptr;
}

CLoginServer::~CLoginServer()
{
	if (_pLoginLanServer != nullptr)
		delete _pLoginLanServer;

	if (_pDBAccount != nullptr)
		delete _pDBAccount;
	
	if (_pMonitorClient != nullptr)
		delete _pMonitorClient;
}

bool CLoginServer::InitPDH()
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
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_ERROR, L"GetModuleFileName failed. Error %d", GetLastError());
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

bool CLoginServer::Start()
{

	_pDBAccount = new CDBAccount(CConfig::_szDBIP, CConfig::_szDBAccount, CConfig::_szDBPassword, CConfig::_szDBName, CConfig::_iDBPort);
	_pDBAccount->ReadDB(CDBAccount::en_ACCOUNT_STATUS_INIT);

	int iTry = 0;
	_pMonitorClient = new CMonitorClient();
	while (!_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false))
	{
		_pMonitorClient->Stop(true);
		if (iTry > 3)
			break;

		++iTry;
	}

	_pLoginLanServer = new CLoginLanServer(this);
	if (!_pLoginLanServer->Start(CConfig::_szLanBindIP, CConfig::_iLanBindPort, CConfig::_iWorkerThreadNo/2, false, 3))
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_ERROR, L"LanServer Start Error");
		return false;
	}

	if (!CNetServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, CConfig::_iWorkerThreadNo/2, false, CConfig::_iClientMax, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_ERROR, L"NetServer Start Error");
		return false;
	}
	return true;
}

void CLoginServer::Stop()
{
	CNetServer::Stop();
}

void CLoginServer::Monitoring()
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

	///////////////////////////////////////////////////
	// Login ó�� ��� �ҿ� �ð�
	ULONGLONG lLoginSuccessTime_Avr = 0;
	ULONGLONG lLoginSuccessTime_Min;

	if (_lLoginRequestCnt > 2)
		lLoginSuccessTime_Avr = (_lLoginSuccessTime_Cnt - _lLoginSuccessTime_Min - _lLoginSuccessTime_Max) / (_lLoginRequestCnt - 2);
	
	lLoginSuccessTime_Min = _lLoginSuccessTime_Min;
	if (_lLoginSuccessTime_Min == 9999999)
	{
		lLoginSuccessTime_Avr = _lLoginSuccessTime_Max;
		lLoginSuccessTime_Min = 0;
	}
	/////////////////////////////////////////////////////

	wprintf(L"===========================================\n");
	wprintf(L" Login Net Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - LoginSuccess TPS	: %lld \n", _lLoginSuccessTps);		
	//wprintf(L" - LoginWait		: %d \n");
	wprintf(L"\n");
	wprintf(L" - CompleteTime Avr	: %lld \n", lLoginSuccessTime_Avr);
	wprintf(L"	Min / Max	: %lld / %lld ms\n", lLoginSuccessTime_Min, _lLoginSuccessTime_Max);

	CNetServer::PrintState(true, true, true, true);
	//_pLoginLanServer->PrintState(true);

	wprintf(L"\n");
	wprintf(L" - CPU Usage		: %d\n", (int)_CPUTime.ProcessTotal());
	wprintf(L" - Commit Memory	: %dM\n", (int)dPrivateMemory);

	if (!_pMonitorClient->_bConnection)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"MonitorServer Reconnect");
		_pMonitorClient->Stop(true);
		_pMonitorClient->Start(CConfig::_szMonitoringLanIP, CConfig::_iMonitoringLanPort, 1, false);
	}
	else
	{

		//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // ��ġ����ŷ ���� ON
		//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // ��ġ����ŷ CPU ���� (Ŀ�� + ����)
		//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // ��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
		//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // ��ġ����ŷ ��ŶǮ ��뷮
		//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // ��ġ����ŷ ���� ����
		//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // ��ġ����ŷ ���� ���� (�α��� ���� ��)
		//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // ��ġ����ŷ �� ���� ���� �� (�ʴ�)
		int iUsePacketCnt = CNPacket::GetUseSize();
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_SERVER_ON, TRUE);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_CPU, _CPUTime.ProcessTotal());
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT, Counter.doubleValue / 1024 / 1024);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL, iUsePacketCnt);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_SESSION, _lConnectCnt);
		//_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_PLAYER, _lLoginSuccessCnt);
		_pMonitorClient->SendData(dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS, _lLoginSuccessTps);
	}
	_lLoginSuccessTps = 0;
}

bool CLoginServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CLoginServer::OnClientJoin(UINT64 SessionID)
{
	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	pPlayer->byStatus = dfLOGIN_STATUS_NONE;
	//pPlayer->iAccountNo = 0;
	pPlayer->iSessionID = SessionID;
	pPlayer->lLoginReqTick = -1;
	ZeroMemory(pPlayer->szID, sizeof(pPlayer->szID));
	ZeroMemory(pPlayer->szNickname, sizeof(pPlayer->szNickname));
	ZeroMemory(pPlayer->szSessionKey, sizeof(pPlayer->szSessionKey));

	AcquireSRWLockExclusive(&_srwPlayerLock);
	_PlayerList.push_back(pPlayer);
	ReleaseSRWLockExclusive(&_srwPlayerLock);
}

void CLoginServer::OnClientLeave(UINT64 SessionID)
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

void CLoginServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		proc_PACKET_CS_LOGIN_REQ_LOGIN(SessionID, pPacket);
		break;
	default:
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		break;
	}
}

void CLoginServer::OnSend(UINT64 SessionID, int iSendSize)
{
	//st_PLAYER* pPlayer = SearchPlayer(SessionID);
	//if (pPlayer == nullptr)
	//{
	//	LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID:%d", SessionID);
	//	return;
	//}
	//if (pPlayer->iSessionID == SessionID)
	//	DisconnectSession(SessionID);
}

void CLoginServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CLoginServer::OnHeartBeat()
{
}

CLoginServer::st_PLAYER * CLoginServer::SearchPlayer(UINT64 iSessionID)
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

void CLoginServer::proc_PACKET_CS_LOGIN_REQ_LOGIN(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	BYTE byStatus = 1;
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID:%d", iSessionID);
		return;
	}

	*pPacket >> pPlayer->iAccountNo;
	pPacket->GetData((char*)pPlayer->szSessionKey, sizeof(pPlayer->szSessionKey));

	InterlockedIncrement64(&_lLoginRequestCnt);

	// ä�� ������ ���� ���� Ȯ��
	if (!_bConnectChatServer)
	{
		pPlayer->byStatus = dfLOGIN_STATUS_NOSERVER;

		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		return;
	}


	// �α��� ��û �ð�
	pPlayer->lLoginReqTick = GetTickCount64();

	// TODO : DB
	CDBAccount::st_SESSIONCHACK_IN pIn;
	pIn.AccountNo = pPlayer->iAccountNo;
	pIn.SessionKey = pPlayer->szSessionKey;

	CDBAccount::st_SESSIONCHACK_OUT pOut;
	pOut.ID = pPlayer->szID;
	pOut.Nickname = pPlayer->szNickname;

	_pDBAccount->ReadDB(CDBAccount::en_ACCOUNT_SESSION_CHECK, (void*)&pIn, (void*)&pOut);
	
	pPlayer->byStatus = pOut.byStatus;
	if (pPlayer->byStatus != dfLOGIN_STATUS_OK)
	{
		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		return;
	}
	
	//------------------------------------------------------------
	// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
	//
	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		CHAR	SessionKey[64]
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,
	_pLoginLanServer->proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(pPlayer->iAccountNo, pPlayer->szSessionKey, pPlayer->iSessionID);
}

void CLoginServer::comp_PACKET_SS_RES_NEW_CLIENT_LOGIN(BYTE byServerType, INT64 iAccountNo, INT64 iParameter)
{
	//------------------------------------------------------------
	// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
	//
	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		CHAR	SessionKey[64]
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
	st_PLAYER *pPlayer = SearchPlayer(iParameter);
	if (pPlayer == nullptr)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID(Parameter):%d", iParameter);
		return;
	}
	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player SessionID Error - %d != %d", pPlayer->iAccountNo, iAccountNo);
		return;
	}

	if (byServerType == dfSERVER_TYPE_CHAT)
	{
		InterlockedIncrement64(&_lLoginSuccessTps);
		pPlayer->byStatus = dfLOGIN_STATUS_OK;

		ULONGLONG lLoginLapseTick = GetTickCount64() - pPlayer->lLoginReqTick;
		_lLoginSuccessTime_Cnt += lLoginLapseTick;
		_lLoginSuccessTime_Max = max(lLoginLapseTick, _lLoginSuccessTime_Max);
		_lLoginSuccessTime_Min = min(lLoginLapseTick, _lLoginSuccessTime_Max);

		// Ŭ���̾�Ʈ�� �α��� ��� ����
		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket_Disconnect(pPlayer->iSessionID, Packet);
		Packet->Free();

		pPlayer->lLoginReqTick = -1;
	}
}

void CLoginServer::mpResLogin(CNPacket * pBuffer, INT64 iAccountNo, BYTE byStatus, WCHAR* szID, WCHAR* szNick)
{
	//------------------------------------------------------------
	// �α��� �������� Ŭ���̾�Ʈ�� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (���ǿ���) / 1 (����) ...  �ϴ� defines ���
	//
	//		WCHAR	ID[20]				// ����� ID		. null ����
	//		WCHAR	Nickname[20]		// ����� �г���	. null ����
	//
	//		WCHAR	GameServerIP[16]	// ���Ӵ�� ����,ä�� ���� ����
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//	}
	//
	//------------------------------------------------------------
	*pBuffer << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
	*pBuffer << iAccountNo;
	*pBuffer << byStatus;
	pBuffer->PutData((char*)szID, sizeof(WCHAR) * 20);
	pBuffer->PutData((char*)szNick, sizeof(WCHAR) * 20);
	// GameServer. �ӽ÷� ChatServer
	pBuffer->PutData((char*)CConfig::_szChatServerIP, sizeof(WCHAR) * 16);
	*pBuffer << (USHORT)CConfig::_iChatServerPort;
	// ChatServer
	pBuffer->PutData((char*)pConfig->_szChatServerIP, sizeof(WCHAR) * 16);
	*pBuffer << (USHORT)CConfig::_iChatServerPort;
}