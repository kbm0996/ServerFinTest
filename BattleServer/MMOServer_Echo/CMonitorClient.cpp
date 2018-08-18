#include "CMonitorClient.h"

CMonitorClient::CMonitorClient()
{
	_bConnection = false;
}

CMonitorClient::~CMonitorClient()
{
}

void CMonitorClient::OnClientJoin()
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
	//	dfMONITOR_SERVER_CONTROL_RUN = 3,		// 서버 프로세스 생성 & 실행
	//};
	CNPacket* pSendPacket = CNPacket::Alloc();

	*pSendPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
	*pSendPacket << (int)dfMONITOR_SERVER_TYPE_GAME;

	WORD wHeader = pSendPacket->GetDataSize();
	pSendPacket->SetHeader_Custom((char*)&wHeader, sizeof(wHeader));

	SendPacket(pSendPacket);
	pSendPacket->Free();

	_bConnection = true;
}

void CMonitorClient::OnClientLeave()
{
	_bConnection = false;
}

void CMonitorClient::OnRecv(CNPacket * pPacket)
{
}

void CMonitorClient::OnSend(int iSendSize)
{
}

void CMonitorClient::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CMonitorClient::SendData(BYTE byType, int iData)
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

	CNPacket* pSendPacket = CNPacket::Alloc();

	*pSendPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pSendPacket << byType;
	*pSendPacket << iData;
	*pSendPacket << (int)time(NULL);

	WORD wHeader = pSendPacket->GetDataSize();
	pSendPacket->SetHeader_Custom((char*)&wHeader, sizeof(wHeader));

	SendPacket(pSendPacket);
	pSendPacket->Free();
}