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
	// �ٸ� ������ �α��� ������ �α���.
	// �̴� ������ ������, �׳� �α��� ��.  
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
	//
	//		WCHAR	ServerName[32]		// �ش� ������ �̸�.  
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
	// ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������Ŷ ���Ű���� ������.
	// ���Ӽ�����, ä�ü����� ��Ŷ�� ������ ������, �α��μ����� Ÿ ������ ���� �� CHAT,GAME ������ �����ϹǷ� 
	// �̸� ����ؼ� �˾Ƽ� ���� �ϵ��� ��.
	//
	// �÷��̾��� ���� �α��� �Ϸ�� �� ��Ŷ�� Chat,Game ���ʿ��� �� �޾��� ������.
	//
	// ������ �� Parameter �� �̹� ����Ű ������ ���� ������ �� �ִ� Ư�� ��
	// ClientID �� ����, ���� ī������ ���� ��� ����.
	//
	// �α��μ����� ���Ӱ� �������� �ݺ��ϴ� ��� ������ ���������� ���� ������ ���� ��������
	// �����Ͽ� �ٸ� ����Ű�� ��� ���� ������ ����.
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
	
	// �ش� ServerType���κ��� �� �Ϸ� ������ NetServer�� ����
	_pLoginNetServer->comp_PACKET_SS_RES_NEW_CLIENT_LOGIN(dfSERVER_TYPE_CHAT, iAccountNo, iParameter);
}

void CLoginLanServer::proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(INT64 iAccountNo, CHAR * szSessionKey, INT64 iParameter)
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
