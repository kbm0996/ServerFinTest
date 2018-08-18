#include "MonitorNetServer.h"
#include "CConfig.h"
#include <conio.h>

bool g_bControlMode = false;;
bool ServerControl();

void main()
{
	timeBeginPeriod(1);
	_wsetlocale(LC_ALL, L"kor");
	g_pConfig->LoadConfigFile("_MonitorServer.cnf");

	CMonitorNetServer MonitorServer;
	if(!MonitorServer.Start())
		return;

	ULONGLONG lUpdateTick = GetTickCount64();
	while (1)
	{
		MonitorServer.Monitoring_Console();
		MonitorServer.UpdatePDH();
		MonitorServer.UpdateClient();
		MonitorServer.TimeOut_LanClient();
		if (GetTickCount64() - lUpdateTick >= CMonitorNetServer::en_DBSAVE_INTERVAL)
		{
			MonitorServer.UpdateDB();
			lUpdateTick = GetTickCount64();
		}
		Sleep(CMonitorNetServer::en_WORKER_SLEEP);
		system("cls");
		if (!ServerControl())
			break;
	}

	MonitorServer.Stop();
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
