#ifndef __SERVER_CONFIG__
#define __SERVER_CONFIG__
#include "Posthttp.h"
#include "CParser.h"

namespace mylib
{
	class CConfig
	{
	private:

		CConfig() {}
		virtual ~CConfig() {}
	public:
		static CConfig* GetInstance()
		{
			static CConfig Instance;
			return &Instance;
		}

		bool LoadConfigFile(char* szConfigFileName)
		{
			CParser Parser;
			if (!Parser.LoadFile(szConfigFileName))
				return false;

			// :NETWORK
			Parser.GetValue("SERVER_NAME", _szServerName, sizeof(_szServerName));

			Parser.GetValue("BIND_IP", _szBindIP, sizeof(_szBindIP));
			Parser.GetValue("BIND_PORT", &_iBindPort);
			Parser.GetValue("LAN_BIND_IP", _szLanBindIP, sizeof(_szLanBindIP));
			Parser.GetValue("LAN_BIND_PORT", &_iLanBindPort);

			// :SYSTEM
			Parser.GetValue("LOGIN_KEY", _szLoginSessionKey, sizeof(_szLoginSessionKey));
			//printf("Key: %s\n", _szLoginSessionKey);


			Parser.GetValue("PACKET_CODE", &_byPacketCode);
			Parser.GetValue("PACKET_KEY1", &_byPacketKey1);
			Parser.GetValue("PACKET_KEY2", &_byPacketKey2);

			Parser.GetValue("DB_IP", _szDBIP, sizeof(_szDBIP));
			Parser.GetValue("DB_ACCOUNT", _szDBAccount, sizeof(_szDBAccount));
			Parser.GetValue("DB_PORT", &_iDBPort);
			Parser.GetValue("DB_PASS", _szDBPassword, sizeof(_szDBPassword));
			Parser.GetValue("DB_NAME", _szDBName, sizeof(_szDBName));

			return true;
		}

		// :NETWORK
		//-----------------------------------
		// 본 서버군 이름 / ServerLink 정보의 이름과 같아야 함
		//-----------------------------------
		static WCHAR	_szServerName[64];

		//-----------------------------------
		// 로그인서버 Listen IP/PORT
		//
		// 이는 서버에서 클라이언트 접속용 Listen 소켓 Bind
		//-----------------------------------
		static WCHAR	_szBindIP[16];
		static int		_iBindPort;
		static WCHAR	_szLanBindIP[16];
		static int		_iLanBindPort;

		//:SYSTEM
		//-----------------------------------
		// 모니터링서버 로그인 키
		//-----------------------------------
		static char		_szLoginSessionKey[33];

		//-----------------------------------
		// Packet Encode Key
		//-----------------------------------
		static int		_byPacketCode;
		static int		_byPacketKey1;
		static int		_byPacketKey2;
		
		static int		_AutoConnect;

		//-----------------------------------
		// DB
		//-----------------------------------
		static WCHAR	_szDBIP[16];
		static WCHAR	_szDBAccount[64];
		static int		_iDBPort;
		static WCHAR	_szDBPassword[64];
		static WCHAR	_szDBName[64];
	};
}

extern mylib::CConfig* g_pConfig;
#endif