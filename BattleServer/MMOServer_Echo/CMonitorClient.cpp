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
	// LoginServer, GameServer , ChatServer , Agent �� ����͸� ������ �α��� ��
	//
	// 
	//	{
	//		WORD	Type
	//
	//		int		ServerNo		// ���� Ÿ�� ���� �� �������� ���� ��ȣ�� �ο��Ͽ� ���
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

	//	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// ���� �������� (���Ӽ��� ����)
	//	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// ���� ���μ��� ��������
	//	dfMONITOR_SERVER_CONTROL_RUN = 3,		// ���� ���μ��� ���� & ����
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
	// ������ ����͸������� ������ ����
	// �� ������ �ڽ��� ����͸����� ��ġ�� 1�ʸ��� ����͸� ������ ����.
	//
	// ������ �ٿ� �� ��Ÿ ������ ����͸� �����Ͱ� ���޵��� ���ҋ��� ����Ͽ� TimeStamp �� �����Ѵ�.
	// �̴� ����͸� Ŭ���̾�Ʈ���� ���,�� ����Ѵ�.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
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