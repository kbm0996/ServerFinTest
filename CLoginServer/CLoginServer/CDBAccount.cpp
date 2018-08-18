#include "CDBAccount.h"

CDBAccount::CDBAccount(WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort) : CDBConnector_TLS(szDBIP, szUser, szPassword, szDBName, iDBPort)
{
}

CDBAccount::~CDBAccount()
{
}

bool CDBAccount::ReadDB(BYTE byType, void * pIn, void * pOut)
{
	switch (byType)
	{
	case en_ACCOUNT_STATUS_INIT:
		proc_ACCOUNT_STATUS_INIT();
		break;
	case en_ACCOUNT_SESSION_CHECK:
		proc_ACCOUNT_SESSION_CHECK((st_SESSIONCHACK_IN*)pIn, (st_SESSIONCHACK_OUT*)pOut);
		break;
	default:
		return false;
	}
	return true;
}

bool CDBAccount::WriteDB(INT64 iAccountNo, bool bSuccessFlag)
{
	if (bSuccessFlag)
	{
		if (!Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='1' WHERE `accountno` = '%lld'", iAccountNo))
			return false;
		return true;
	}
	else
	{
		if (!Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='0' WHERE `accountno` = '%lld'", iAccountNo))
			return false;
		return true;
	}
}

void CDBAccount::proc_ACCOUNT_STATUS_INIT()
{
	Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='0'");
}

void CDBAccount::proc_ACCOUNT_SESSION_CHECK(st_SESSIONCHACK_IN * pIn, st_SESSIONCHACK_OUT * pOut)
{
	// AccountNo�� ID, Nickname ���
	MYSQL_ROW Row;

	// �����̺� �����;��
	Query(L"SELECT * FROM `accountdb`.`v_account` WHERE `accountno` = '%lld'", pIn->AccountNo);
	Row = FetchRow();
	if (Row == nullptr)
	{
		FreeResult();
		LOG(L"DBAccount", LOG_ERROR, L"%lld Account Not Exist in DB", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_ACCOUNT_MISS;
		return;
	}

	// �信�� �о�� ���� 1���� null�̶�� ����
	// ����Ű�� �Ʒ����� Ȯ��
	if (Row[1] == nullptr || Row[2] == nullptr || Row[4] == nullptr)
	{
		FreeResult();
		LOG(L"DBAccount", LOG_ERROR, L"%lld Account Player Not Exist in DB", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_STATUS_MISS;
		return;
	}

	UTF8toUTF16(Row[1], pOut->ID, 20);
	UTF8toUTF16(Row[2], pOut->Nickname, 20);

	// ����Ű�� ���ų� ��ġ���� �ʴ´ٸ�
	if (pIn->AccountNo >= 1000000)
	{
		if (memcmp(Row[3], pIn->SessionKey, 64) != 0)
		{
			FreeResult();
			LOG(L"DBAccount", LOG_DEBUG, L"%lld Account Player SessionKey Missmatch", pIn->AccountNo);
			pOut->byStatus = dfLOGIN_STATUS_FAIL;
			return;
		}
	}

	// ���� Status ���
	int iStatus = atoi(Row[4]);
	FreeResult();
	// �÷��̾ �������̶��
	if (iStatus != 0)
	{
		//LOG(L"DBAccount", LOG_DEBUG, L"%lld Account Player In Game Playing", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_GAME;
		return;
	}

	pOut->byStatus = dfLOGIN_STATUS_OK;
}
