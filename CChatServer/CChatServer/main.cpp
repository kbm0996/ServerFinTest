#include "CChatServer.h"
#include "CConfig.h"
#include <conio.h>

bool g_bControlMode = false;
bool ServerControl();

void main()
{
	timeBeginPeriod(1);
	_wsetlocale(LC_ALL, L"kor");
	g_pConfig->LoadConfigFile("_ChatServer.cnf");

	CChatServer ChatServer;
	if (!ChatServer.Start())
		return;

	while (1)
	{
		ChatServer.Monitoring();
		Sleep(1000);
		system("cls");
		if (!ServerControl())
			break;
	}

	ChatServer.Stop();
	timeEndPeriod(1);
}

bool ServerControl()
{
	//------------------------------------------
	// L : Control Lock / U : Unlock / Q : Quit
	//------------------------------------------
	//  _kbhit() 함수 자체가 느리기 때문에 사용자 혹은 더미가 많아지면 느려져서 실제 테스트시 주석처리 권장
	// 그런데도 GetAsyncKeyState를 안 쓴 이유는 창이 활성화되지 않아도 키를 인식함 Windowapi의 경우 
	// 제어가 가능하나 콘솔에선 어려움

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			g_bControlMode = true;

			wprintf(L"[ Control Mode ] \n");
			wprintf(L"Press  L	- Key Lock \n");
			wprintf(L"Press  Q	- Quit \n");
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
		}

	}
	return true;
}