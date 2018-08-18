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
	// OnConnectRequest		: Accept 직후, true/false 접속 허용/거부
	// OnClientJoin			: Accept 접속처리 완료 후, 유저 접속 관련
	// OnClientLeave		: Disconnect 후, 유저 정리
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnError				: 에러 발생 후
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
	//dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // 매치메이킹 서버 ON
	int	_bOnLoginServer;
	//dfMONITOR_DATA_TYPE_MATCH_CPU,                              // 매치메이킹 CPU 사용률 (커널 + 유저)
	int	_iCpuUsage_Login;
	//dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // 매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
	int _iCommitMemory_Login;
	//dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // 매치메이킹 패킷풀 사용량
	int _iPacketPool_Login;
	//dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // 매치메이킹 접속 세션
	int _iSessionAll_Login;
	//dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // 매치메이킹 접속 유저 (로그인 성공 후)
	//int _iSessionSuccess_Login;
	//dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // 매치메이킹 방 배정 성공 수 (초당)
	int _iLoginSuccessTps_Login;

	//////////////////////////////////////////////////////////////////////////
	// BattleServer
	//////////////////////////////////////////////////////////////////////////
	//dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // 배틀서버 ON
	int	_bOnGameServer;				
	//dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // 배틀서버 CPU 사용률 (커널 + 유저)
	int	_iCpuUsage_Game;			
	//dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
	int _iCommitMemory_Game;
	//dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // 배틀서버 패킷풀 사용량
	int _iPacketPool_Game;          
	//dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // 배틀서버 Auth 스레드 초당 루프 수
	int _iAuthFps_Game;			   
	//dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // 배틀서버 Game 스레드 초당 루프 수
	int _iGameFps_Game;			   
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // 배틀서버 접속 세션전체
	int _iSessionAll_Game;			
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // 배틀서버 Auth 스레드 모드 인원
	int _iSessionAuth_Game;         
	//dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // 배틀서버 Game 스레드 모드 인원
	int _iSessionGame_Game;
	//dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // 배틀서버 대기방 수
	//dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // 배틀서버 플레이방 수

	//////////////////////////////////////////////////////////////////////////
	// ChatServer
	//////////////////////////////////////////////////////////////////////////
	//dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // 채팅서버 ON
	int	_bOnChatServer;
	//dfMONITOR_DATA_TYPE_CHAT_CPU,                               // 채팅서버 CPU 사용률 (커널 + 유저)
	int	_iCpuUsage_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
	int _iCommitMemory_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // 채팅서버 패킷풀 사용량
	int _iPacketPool_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // 채팅서버 접속 세션전체
	int _iSessionAll_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // 채팅서버 로그인을 성공한 전체 인원
	int _iSessionLogin_Chat;
	//dfMONITOR_DATA_TYPE_CHAT_ROOM                               // 배틀서버 방 수
};

#endif