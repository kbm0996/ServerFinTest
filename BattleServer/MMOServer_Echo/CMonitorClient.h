#ifndef __MONITORING_CLIENT__
#define __MONITORING_CLIENT__

#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "CLanClient.h"
#include "MonitorProtocol.h"

using namespace mylib;
using namespace std;

class CMonitorClient : public CLanClient
{
public:
	CMonitorClient();
	virtual ~CMonitorClient();

private:
	virtual void OnClientJoin();
	virtual void OnClientLeave();
	virtual void OnRecv(CNPacket * pPacket);
	virtual void OnSend(int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);

public:
	void SendData(BYTE byType, int iData);
	bool _bConnection;
};
#endif
