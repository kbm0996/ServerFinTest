#ifndef __DB_ACCOUNT__
#define __DB_ACCOUNT__

// TODO: other code

#pragma comment(lib, "../../../MyLibrary/MyLib/x64/Release/MyLib.lib")
#include "MyLib.h"
#include "CDBConnector_TLS.h"
#include "MonitorProtocol.h"
#include "Posthttp.h"

using namespace mylib;

class CDBAccount : public CDBConnector_TLS
{
public:
	struct st_DB_INDATA
	{
		INT64	AccountNo;
		char*	SessionKey;
	};

	struct st_DB_OUTDATA
	{
		WCHAR*	ID;
		WCHAR*	Nickname;
		BYTE	byStatus;
	};

	CDBAccount(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
	virtual ~CDBAccount();

	bool InitDB();
	bool ReadDB(st_DB_INDATA* pIn, st_DB_OUTDATA* pOut);
	bool WriteDB(INT64 iAccountNo, bool bSuccessFlag);
};

#endif