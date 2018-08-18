#ifndef __MONITOR_LANSERVER_LOG__
#define __MONITOR_LANSERVER_LOG__

#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CLanServer.h"
#include "MonitorProtocol.h"
#include <list>

using namespace std;
using namespace mylib;

class CMonitorLanServer : public CLanServer
{
public:
	enum en_SERVER_CONFIG
	{
		en_SERVER_TIMEOUT = 30000
	};

	struct st_SERVER
	{
		UINT64		iSessionID;
		int			iServerNo;
		int			iUpdateTime;
		ULONGLONG	lLastRecvTick;
	};

	CMonitorLanServer();
	virtual ~CMonitorLanServer();

	void	ServerList_Timeout();

protected:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnConnectRequest		: Accept ����, true/false ���� ���/�ź�
	// OnClientJoin			: Accept ����ó�� �Ϸ� ��, ���� ���� ����
	// OnClientLeave		: Disconnect ��, ���� ����
	// OnRecv				: ��Ŷ ���� ��, ��Ŷ ó��
	// OnSend				: ��Ŷ �۽� ��
	// OnError				: ���� �߻� ��
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnConnectRequest(WCHAR* wszIP, int iPort);
	virtual void OnClientJoin(UINT64 SessionID);
	virtual void OnClientLeave(UINT64 SessionID);
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);

	st_SERVER*	SearchServer(UINT64 iSessionID);

	// OnRecv
	void proc_PACKET_SS_MONITOR_LOGIN(UINT64 SessionID, CNPacket * pPacket);
	void proc_PACKET_SS_MONITOR_DATA_UPDATE(UINT64 SessionID, CNPacket * pPacket);

private:
	// Server
	list<st_SERVER*>	_ServerList;
	SRWLOCK				_srwServerLock;

public:
	//////////////////////////////////////////////////////////////////////////
	// MatchServer
	//////////////////////////////////////////////////////////////////////////
	//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // ��ġ����ŷ ���� ON
	int	_bOnLoginServer;
	//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // ��ġ����ŷ CPU ���� (Ŀ�� + ����)
	int	_iCpuUsage_Login;
	//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // ��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
	int _iCommitMemory_Login;
	//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // ��ġ����ŷ ��ŶǮ ��뷮
	int _iPacketPool_Login;
	//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // ��ġ����ŷ ���� ����
	int _iSessionAll_Login;
	//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // ��ġ����ŷ ���� ���� (�α��� ���� ��)
	//int _iSessionSuccess_Login;
	//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // ��ġ����ŷ �� ���� ���� �� (�ʴ�)
	int _iLoginSuccessTps_Login;

	//////////////////////////////////////////////////////////////////////////
	// BattleServer
	//////////////////////////////////////////////////////////////////////////
	//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // ��Ʋ���� ON
	int	_bOnGameServer;				
	//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // ��Ʋ���� CPU ���� (Ŀ�� + ����)
	int	_iCpuUsage_Game;			
	//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte
	int _iCommitMemory_Game;
	//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // ��Ʋ���� ��ŶǮ ��뷮
	int _iPacketPool_Game;          
	//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // ��Ʋ���� Auth ������ �ʴ� ���� ��
	int _iAuthFps_Game;			   
	//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // ��Ʋ���� Game ������ �ʴ� ���� ��
	int _iGameFps_Game;			   
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // ��Ʋ���� ���� ������ü
	int _iSessionAll_Game;			
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // ��Ʋ���� Auth ������ ��� �ο�
	int _iSessionAuth_Game;         
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // ��Ʋ���� Game ������ ��� �ο�
	int _iSessionGame_Game;
	//dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // ��Ʋ���� ���� ��
	//dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // ��Ʋ���� �÷��̹� ��

	//////////////////////////////////////////////////////////////////////////
	// ChatServer
	//////////////////////////////////////////////////////////////////////////
	//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // ä�ü��� ON
	int	_bOnChatServer;
	//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // ä�ü��� CPU ���� (Ŀ�� + ����)
	int	_iCpuUsage_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // ä�ü��� �޸� ���� Ŀ�� ��뷮 (Private) MByte
	int _iCommitMemory_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // ä�ü��� ��ŶǮ ��뷮
	int _iPacketPool_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // ä�ü��� ���� ������ü
	int _iSessionAll_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // ä�ü��� �α����� ������ ��ü �ο�
	int _iSessionLogin_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_ROOM                               // ��Ʋ���� �� ��
};

#endif