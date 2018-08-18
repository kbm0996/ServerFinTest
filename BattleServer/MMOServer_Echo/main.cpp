#include "CBattleServer.h"
#include <conio.h>
#include "CConfig.h"

bool g_bControlMode = false;;
bool ServerControl();

void main()
{
	timeBeginPeriod(1);

	_wsetlocale(LC_ALL, L"kor");
	g_pConfig->LoadConfigFile("_BattleServer.cnf");

	CBattleServer EchoServer(CConfig::_iClientMax);
	if (!EchoServer.Start())
		return;

	while (1)
	{
		EchoServer.Monitoring();
		Sleep(1000);
		system("cls");
		if (!ServerControl())
			break;
	}

	EchoServer.Stop();
	timeEndPeriod(1);
}

bool ServerControl()
{
	//------------------------------------------
	// L : Control Lock / U : Unlock / Q : Quit
	//------------------------------------------
	//  _kbhit() �Լ� ��ü�� ������ ������ ����� Ȥ�� ���̰� �������� �������� ���� �׽�Ʈ�� �ּ�ó�� ����
	// �׷����� GetAsyncKeyState�� �� �� ������ â�� Ȱ��ȭ���� �ʾƵ� Ű�� �ν��� Windowapi�� ��� 
	// ��� �����ϳ� �ֿܼ��� �����

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			g_bControlMode = true;

			wprintf(L"[ Control Mode ] \n");
			wprintf(L"Press  L	- Key Lock \n");
			wprintf(L"Press  Q	- Quit \n");
#ifdef __DEBUG__
			wprintf(L"Press  D	- MP TLS Thread tracing \n");
#endif
		}

		if (g_bControlMode == true)
		{
			if (L'l' == ControlKey || L'L' == ControlKey)
			{
				wprintf(L"Controll Lock. Press U - Control Unlock \n");
				g_bControlMode = false;
			}

			if (L'q' == ControlKey || L'Q' == ControlKey)
			{
				return false;
			}
		#ifdef __DEBUG__
			if (L'd' == ControlKey || L'D' == ControlKey)
			{
				int i = 0;
				auto iter = CNPacket::_PacketPool._DebugChunkMap.begin();
				for (; iter != CNPacket::_PacketPool._DebugChunkMap.end(); ++iter)
				{
					++i;
					wprintf(L"%d (%d) : %d /// ", i, (*iter).first->_iFreeCount, (*iter).second);

					if (i % 5 == 0)
						wprintf(L"\n");
					
				}
				wprintf(L"\n");
			}
		#endif
		}

	}
	return true;
}