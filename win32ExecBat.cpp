#include <Windows.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("Param 1: exe name, Param 2: exec num\n");
		system("pause");
		return 0;
	}
	char pExeName[32] = { 0 };
	strncpy_s(pExeName, argv[1], strlen(argv[1]) + 1);
	printf("ExeName: %s\n", pExeName);
	int iNum = atoi(argv[2]);
	printf("Num: %d\n", iNum);
	char pCurrentPath[128];
	GetCurrentDirectory(128, pCurrentPath);

	for (int i = 0; i < iNum; i++)
	{
		char	pBuffer[64];
		sprintf_s(pBuffer, "%s\\%s.exe", pCurrentPath, pExeName);
		printf("startup: %s\n", pBuffer);

		STARTUPINFO si;
		memset(&si, 0, sizeof(STARTUPINFO));
		PROCESS_INFORMATION pi;
		CreateProcess(NULL, pBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	}
	Sleep(INFINITE);
	return 0;
}