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
	// LoginList 삭제 조건
	// 1. 채팅 서버 접속 완료시
	// 2. 주기적으로 삭제(대략 30초. 채팅 서버로 접속 안하고 로그인 서버까지만 출입하는 유저)
	// 3. AccountNo는 같은데 SessionKEy가 다른 경우? 정책에 따라 알아서 처리
	st_LOGIN_INFO* pInfo = _LogininfoPool->Alloc();

	*pPacket >> pInfo->iAccountNo;
	pPacket->GetData(pInfo->szSessionKey, sizeof(st_LOGIN_INFO::szSessionKey));
	*pPacket >> pInfo->iParameter;

	pInfo->lLastRecvTick = GetTickCount64();

	// LoginList 추가
	LoginMap_Add(pInfo);

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
	CNPacket *pSendPacket = CNPacket::Alloc();
	*pSendPacket << (WORD)en_PACKET_SS_RES_NEW_CLIENT_LOGIN;
	*pSendPacket << pInfo->iAccountNo;
	*pSendPacket << pInfo->iParameter;

	// LanServer, LanClient 간 통신 패킷의 헤더 = 페이로드 길이
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

	// 중복일 경우 삭제
	if (pInfo != nullptr)
	{
		//LOG(L"LOGIN_CLIENT_LOG", LOG_WARNG, L"No%d Account Overlapped Login", pInfo->iAccountNo);
		_LoginMap.erase(pAddInfo->iAccountNo);
		_LogininfoPool->Free(pInfo);
	}

	// 삽입
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
