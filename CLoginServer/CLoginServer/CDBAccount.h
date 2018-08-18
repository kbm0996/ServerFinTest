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
	enum en_AccountType
	{
		en_ACCOUNT_STATUS_INIT = 0,
		en_ACCOUNT_SESSION_CHECK,
	};
	struct st_SESSIONCHACK_IN
	{
		INT64	AccountNo;
		char	*SessionKey;
	};

	struct st_SESSIONCHACK_OUT
	{
		WCHAR	*ID;
		WCHAR	*Nickname;
		BYTE	byStatus;
	};

	CDBAccount(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
	virtual ~CDBAccount();

	bool ReadDB(BYTE byType, void *pIn = nullptr, void *pOut = nullptr);	// ������ �бⰡ�ƴ� ����� ���� ���Ѵٸ� ���⵵ �����ϴ�.
	bool WriteDB(INT64 iAccountNo, bool bSuccessFlag);	// �б� �������� ������ �����

private:
	void proc_ACCOUNT_STATUS_INIT();
	void proc_ACCOUNT_SESSION_CHECK(st_SESSIONCHACK_IN *pIn, st_SESSIONCHACK_OUT *pOut);
};

#endif