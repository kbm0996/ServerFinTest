#ifndef __SERVER_CONFIG__
#define __SERVER_CONFIG__
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

			Parser.GetValue("LOGIN_SERVER_IP", _szLoginServerIP, sizeof(_szLoginServerIP));
			Parser.GetValue("LOGIN_SERVER_PORT", &_iLoginServerPort);

			Parser.GetValue("LOGIN_SERVER_IP", _szLoginLanServerIP, sizeof(_szLoginLanServerIP));
			Parser.GetValue("LOGIN_LAN_SERVER_PORT", &_iLoginLanServerPort);

			Parser.GetValue("MONITORING_SERVER_IP", _szMonitoringLanIP, sizeof(_szMonitoringLanIP));
			Parser.GetValue("MONITORING_SERVER_PORT", &_iMonitoringLanPort);

			Parser.GetValue("WORKER_THREAD", &_iWorkerThreadNo);

			// :SYSTEM
			Parser.GetValue("CLIENT_MAX", &_iClientMax);

			Parser.GetValue("PACKET_CODE", &_byPacketCode);
			Parser.GetValue("PACKET_KEY1", &_byPacketKey1);
			Parser.GetValue("PACKET_KEY2", &_byPacketKey2);


			return true;
		}

		// :NETWORK
		//-----------------------------------
		// �� ������ �̸� / ServerLink ������ �̸��� ���ƾ� ��
		//-----------------------------------
		static WCHAR	_szServerName[20];

		//-----------------------------------
		// ä�ü��� Listen IP/PORT
		//
		// �̴� �������� Ŭ���̾�Ʈ ���ӿ� Listen ���� Bind
		//-----------------------------------
		static WCHAR	_szBindIP[16];
		static int		_iBindPort;

		//-----------------------------------
		// �α��μ��� ���� IP/PORT
		//-----------------------------------
		static WCHAR	_szLoginServerIP[16];
		static int		_iLoginServerPort;

		static WCHAR	_szLoginLanServerIP[16];
		static int		_iLoginLanServerPort;

		//-----------------------------------
		// ����͸� ���� ���� IP/PORT
		//-----------------------------------
		static WCHAR	_szMonitoringLanIP[16];
		static int		_iMonitoringLanPort;

		//-----------------------------------
		// IOCP ��Ŀ������ ����
		//-----------------------------------
		static int		_iWorkerThreadNo;


		//:SYSTEM
		//-----------------------------------
		// �ִ�����
		//-----------------------------------
		static int		_iClientMax;

		//-----------------------------------
		// Packet Encode Key
		//-----------------------------------
		static int		_byPacketCode;
		static int		_byPacketKey1;
		static int		_byPacketKey2;
	};
}

extern mylib::CConfig* g_pConfig;

#endif