#include "CLoginClient.h"

using namespace mylib;
CLoginClient::CLoginClient(CChatServer* pChatServer)
{
	_pChatServer = pChatServer;

	InitializeSRWLock(&_srwLoginMap);
	_LogininfoPool = new CLFMemoryPool<st_LOGIN_INFO>();

	_bConnection = false;
}

CLoginClient::~CLoginClient()
{
}

void CLoginClient::OnClientJoin()
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
	CNPacket *pPacket = CNPacket::Alloc();
	*pPacket << (WORD)en_PACKET_SS_LOGINSERVER_LOGIN;
	*pPacket << (BYTE)dfSERVER_TYPE_CHAT;
	pPacket->PutData((char*)g_pConfig->_szServerName, sizeof(g_pConfig->_szServerName));

	WORD wHeader = pPacket->GetDataSize();
	pPacket->SetHeader_Custom((char*)&wHeader, sizeof(wHeader));

	SendPacket(pPacket);
	pPacket->Free();

	_bConnection = true;
}

void CLoginClient::OnClientLeave()
{
	_bConnection = false;
}

void CLoginClient::OnRecv(CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_SS_REQ_NEW_CLIENT_LOGIN:
		proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(pPacket);
		break;
	default:
		LOG(L"LOGIN_CLIENT_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		Disconnect();
		break;
	}
}

void CLoginClient::OnSend(int iSendSize)
{
}

void CLoginClient::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CLoginClient::proc_PACKET_SS_REQ_NEW_CLIENT_LOGIN(CNPacket * pPacket)
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
	// LoginList ���� ����
	// 1. ä�� ���� ���� �Ϸ��
	// 2. �ֱ������� ����(�뷫 30��. ä�� ������ ���� ���ϰ� �α��� ���������� �����ϴ� ����)
	// 3. AccountNo�� ������ SessionKEy�� �ٸ� ���? ��å�� ���� �˾Ƽ� ó��
	st_LOGIN_INFO* pInfo = _LogininfoPool->Alloc();

	*pPacket >> pInfo->iAccountNo;
	pPacket->GetData(pInfo->szSessionKey, sizeof(st_LOGIN_INFO::szSessionKey));
	*pPacket >> pInfo->iParameter;

	pInfo->lLastRecvTick = GetTickCount64();

	// LoginList �߰�
	LoginMap_Add(pInfo);

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
	CNPacket *pSendPacket = CNPacket::Alloc();
	*pSendPacket << (WORD)en_PACKET_SS_RES_NEW_CLIENT_LOGIN;
	*pSendPacket << pInfo->iAccountNo;
	*pSendPacket << pInfo->iParameter;

	// LanServer, LanClient �� ��� ��Ŷ�� ��� = ���̷ε� ����
	WORD wHeader = pSendPacket->GetDataSize();
	pSendPacket->SetHeader_Custom((char*)&wHeader, sizeof(wHeader));

	SendPacket(pSendPacket);

	pSendPacket->Free();
}

CLoginClient::st_LOGIN_INFO * CLoginClient::LoginMap_Find(INT64 iAccountNo)
{
	auto Iter = _LoginMap.find(iAccountNo);
	if (Iter == _LoginMap.end())
		return nullptr;

	///wprintf(L"find : %d = %d,%d \n", iAccountNo, (*Iter).first,(*Iter).second->iAccountNo);
	return (*Iter).second;
}

bool CLoginClient::LoginMap_Add(st_LOGIN_INFO * pAddInfo)
{
	AcquireSRWLockExclusive(&_srwLoginMap);

	st_LOGIN_INFO* pInfo = LoginMap_Find(pAddInfo->iAccountNo);

	// �ߺ��� ��� ����
	if (pInfo != nullptr)
	{
		//LOG(L"LOGIN_CLIENT_LOG", LOG_WARNG, L"No%d Account Overlapped Login", pInfo->iAccountNo);
		_LoginMap.erase(pAddInfo->iAccountNo);
		_LogininfoPool->Free(pInfo);
	}

	// ����
	_LoginMap.insert(pair<INT64, st_LOGIN_INFO*>(pAddInfo->iAccountNo, pAddInfo));

	ReleaseSRWLockExclusive(&_srwLoginMap);

	return true;
}

bool CLoginClient::LoginMap_Remove(INT64 iAccountNo, char * szSessionKey)
{
	AcquireSRWLockExclusive(&_srwLoginMap);
	
	st_LOGIN_INFO* pInfo = LoginMap_Find(iAccountNo);
	if (pInfo == nullptr)
	{
		LOG(L"LOGIN_CLIENT_LOG", LOG_WARNG, L"Unknown %d Account Connection Try", iAccountNo);
		ReleaseSRWLockExclusive(&_srwLoginMap);
		return false;
	}

	if (memcmp(pInfo->szSessionKey, szSessionKey, sizeof(st_LOGIN_INFO::szSessionKey)) != 0)
	{
		char szOriginKey[65] = { 0 };
		char szNewKey[65] = { 0 };

		InterlockedIncrement64(&_pChatServer->_lSessionMissCnt);
		
		memcpy_s(&szOriginKey, sizeof(szOriginKey), pInfo->szSessionKey, sizeof(st_LOGIN_INFO::szSessionKey));
		memcpy_s(&szNewKey, sizeof(szNewKey), szSessionKey, sizeof(st_LOGIN_INFO::szSessionKey));

		LOG(L"LOGIN_CLIENT_LOG", LOG_WARNG, L"%d Account SessionKey Mismatch %s : %s", iAccountNo, szOriginKey, szNewKey);
		ReleaseSRWLockExclusive(&_srwLoginMap);
		return false;
	}

	_LoginMap.erase(pInfo->iAccountNo);
	ReleaseSRWLockExclusive(&_srwLoginMap);

	_LogininfoPool->Free(pInfo);

	return true;
}

void CLoginClient::LoginMap_Timeout()
{
	AcquireSRWLockExclusive(&_srwLoginMap);

	for (auto Iter = _LoginMap.begin(); Iter != _LoginMap.end();)
	{
		st_LOGIN_INFO* pInfo = (*Iter).second;
		if (GetTickCount64() - pInfo->lLastRecvTick > en_INFO_TIMEOUT)
		{
			///LOG(L"LOGIN_CLIENT_LOG", LOG_DEBUG, L"%d Account Timeout", (*Iter).second->iAccountNo);
			Iter = _LoginMap.erase(Iter);
			_LogininfoPool->Free(pInfo);
			continue;
		}
		++Iter;
	}

	ReleaseSRWLockExclusive(&_srwLoginMap);
}

int CLoginClient::LoginMap_Size()
{
	AcquireSRWLockShared(&_srwLoginMap);
	int iSize = _LoginMap.size();
	ReleaseSRWLockShared(&_srwLoginMap);
	return iSize;
}
