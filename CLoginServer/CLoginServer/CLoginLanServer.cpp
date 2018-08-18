#include "CLoginLanServer.h"

CLoginLanServer::CLoginLanServer(CLoginServer *pLoginServer)
{
	InitializeSRWLock(&_srwServerLock);

	_pLoginNetServer = pLoginServer;

}

CLoginLanServer::~CLoginLanServer()
{
}

bool CLoginLanServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CLoginLanServer::OnClientJoin(UINT64 SessionID)
{
	st_SERVER* pServer = new st_SERVER;
	pServer->iSessionID = SessionID;
	
	AcquireSRWLockExclusive(&_srwServerLock);
	_ServerList.push_back(pServer);
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CLoginLanServer::OnClientLeave(UINT64 SessionID)
{
	AcquireSRWLockExclusive(&_srwServerLock);
	st_SERVER* pServer;
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end(); Iter++)
	{
		pServer = *Iter;
		if (pServer->iSessionID == SessionID)
		{
			if (pServer->byServerType == dfSERVER_TYPE_CHAT)
				_pLoginNetServer->_bConnectChatServer = false;

			_ServerList.erase(Iter);
			delete pServer;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CLoginLanServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_SS_LOGINSERVER_LOGIN:
		ReqLoginServerLogin(SessionID, pPacket);
		break;
	case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:
		proc_PACKET_SS_RES_NEW_CLIENT_LOGIN(SessionID, pPacket);
		break;
	default:
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		break;
	}
}

void CLoginLanServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

void CLoginLanServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

CLoginLanServer::st_SERVER * CLoginLanServer::SearchServer(UINT64 iSessionID)
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

void CLoginLanServer::ReqLoginServerLogin(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 다른 서버가 로그인 서버로 로그인.
	// 이는 응답이 없으며, 그냥 로그인 됨.  
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
	//
	//		WCHAR	ServerName[32]		// 해당 서버의 이름.  
	//	}
	//
	//------------------------------------------------------------
	// en_PACKET_SS_LOGINSERVER_LOGIN,
	st_SERVER *pServer = SearchServer(iSessionID);
	if (pServer == nullptr)
	{
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}
	*pPacket >> pServer->byServerType;
	pPacket->GetData((char*)pServer->szServerName, sizeof(pServer->szServerName));

	if (pServer->byServerType == dfSERVER_TYPE_CHAT)
		_pLoginNetServer->_bConnectChatServer = true;
}

void CLoginLanServer::proc_PACKET_SS_RES_NEW_CLIENT_LOGIN(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 게임.채팅 서버가 새로운 클라이언트 접속패킷 수신결과를 돌려줌.
	// 게임서버용, 채팅서버용 패킷의 구분은 없으며, 로그인서버에 타 서버가 접속 시 CHAT,GAME 서버를 구분하므로 
	// 이를 사용해서 알아서 구분 하도록 함.
	//
	// 플레이어의 실제 로그인 완료는 이 패킷을 Chat,Game 양쪽에서 다 받았을 시점임.
	//
	// 마지막 값 Parameter 는 이번 세션키 공유에 대해 구분할 수 있는 특정 값
	// ClientID 를 쓰던, 고유 카운팅을 쓰던 상관 없음.
	//
	// 로그인서버에 접속과 재접속을 반복하는 경우 이전에 공유응답이 새로 접속한 뒤의 응답으로
	// 오해하여 다른 세션키를 들고 가는 문제가 생김.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_RES_NEW_CLIENT_LOGIN,
	st_SERVER *pServer = SearchServer(iSessionID);
	if (pServer == nullptr)
	{
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}

	INT64 iAccountNo;
	INT64 iParameter;
	*pPacket >> iAccountNo;
	*pPacket >> iParameter;
	
	// 해당 ServerType으로부터 온 완료 통지를 NetServer에 전달
	_pLoginNetServer->comp_PACKET_SS_RES_NEW_CLIENT_LOGIN(dfSERVER_TYPE_CHAT, iAccountNo, iParameter);
}

void CLoginLanServer::proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(INT64 iAccountNo, CHAR * szSessionKey, INT64 iParameter)
{
	//------------------------------------------------------------
	// 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
	//
	// 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
	// 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
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
	CNPacket *Packet = CNPacket::Alloc();
	*Packet << (WORD)en_PACKET_SS_REQ_NEW_CLIENT_LOGIN;
	*Packet << iAccountNo;
	Packet->PutData(szSessionKey, 64);
	*Packet << iParameter;

	WORD wHeader = Packet->GetDataSize();
	Packet->SetHeader_Custom((char*)&wHeader, sizeof(wHeader));

	AcquireSRWLockExclusive(&_srwServerLock);
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end(); Iter++)
	{
		SendPacket((*Iter)->iSessionID, Packet);
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
	Packet->Free();
}
