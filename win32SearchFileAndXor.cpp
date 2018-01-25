#include <Windows.h>
#include <iostream>
#include <vector>
using namespace std;

#define		XOR_CHAR		0x66

void getCurrentFolders(string strPath, vector<string>& allPath)
{
	string strSearch = strPath + "\\*";
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(strSearch.c_str(), &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;

	do {
		if (FindFileData.cFileName[0] == '.' && (FindFileData.cFileName[1] == '\0' || (FindFileData.cFileName[1] == '.' && FindFileData.cFileName[2] == '\0')))
			continue;
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			allPath.push_back(strPath + "\\" + FindFileData.cFileName);
			getCurrentFolders(strPath + "\\" + FindFileData.cFileName, allPath);
		}
	} while (FindNextFile(hFind, &FindFileData));
}

void Xor(char* buffer, int fileSize)
{
	for (int i = 0; i < fileSize; i++)
		buffer[i] ^= XOR_CHAR;
}

int modifyFile(string strFile)
{
	int iRet = 0;
	FILE* fp = nullptr;
	fopen_s(&fp, strFile.c_str(), "rb+");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* buffer = (char*)malloc(fileSize);
		if (buffer)
		{
			fread(buffer, fileSize, 1, fp);
			Xor(buffer, fileSize);
			fseek(fp, 0, SEEK_SET);
			fwrite(buffer, fileSize, 1, fp);
			free(buffer);
		}
		else
			iRet = 1;
		fclose(fp);
	}
	else
		iRet = 1;
	return iRet;
}

void getCurrentFiles(string strPath)
{
	string strSearch = strPath + "\\*";
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(strSearch.c_str(), &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;

	do {
		if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			string strFileName = strPath + "\\" + FindFileData.cFileName;
			int pos = strFileName.find_last_of('.');
			if (pos > 0)
			{
				string strEnd = strFileName.substr(pos, strFileName.length());
				if (strEnd == ".png" || strEnd == ".jpg" || strEnd == ".jpeg")
				{
					printf("%s\n", strFileName.c_str());
					modifyFile(strFileName);
				}
			}
		}
	} while (FindNextFile(hFind, &FindFileData));
}

int main(void)
{
	vector<string> allPath;
	allPath.push_back(".");	
	getCurrentFolders(".", allPath);

	for (size_t i = 0; i < allPath.size(); i++)
		getCurrentFiles(allPath.at(i));

	system("pause");
	return 0;
}