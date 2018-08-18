#include <Windows.h>
#include <cstdio>
#include <cwchar>

int main()
{
	WCHAR	szLine[2048] = { 0, };
	FILE*   fpFile;
	if(_wfopen_s(&fpFile, L"ID0.txt", L"wt+,ccs=UNICODE") != 0) 
		return 0;
	
	for (int i = 1; i <= 15001; ++i)
	{
		if (i == 1)
			wsprintf(szLine, L"%d	ID_0	NICK_0\n", i);
		else
			wsprintf(szLine, L"%d	ID_0%d	NICK_0%d\n", i, i, i);
		fputws(szLine, fpFile);
	}

	fclose(fpFile);
	return 0;
}