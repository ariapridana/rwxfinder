#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <Psapi.h>
#include <string.h>

void parsePESection(wchar_t* dllpath) {
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	PBYTE	pBuff = NULL;
	DWORD	dwFileSize = NULL, dwNumberOfBytesRead = NULL;
	hFile = CreateFileW(dllpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto _EndOfFunction;
	}

	dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize == NULL) {
		goto _EndOfFunction;
	}

	pBuff = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize);
	if (pBuff == NULL) {
		goto _EndOfFunction;
	}

	if (!ReadFile(hFile, pBuff, dwFileSize, &dwNumberOfBytesRead, NULL) || dwFileSize != dwNumberOfBytesRead) {
		goto _EndOfFunction;
	}

	PIMAGE_DOS_HEADER pDOS = (PIMAGE_DOS_HEADER)pBuff;
	PIMAGE_NT_HEADERS pImgNtHdrs = (PIMAGE_NT_HEADERS)((PBYTE)pBuff + pDOS->e_lfanew);
	if (pDOS->e_magic != IMAGE_DOS_SIGNATURE) {
		return;
	}
	if (pImgNtHdrs->Signature != IMAGE_NT_SIGNATURE) {
		return;
	}
	IMAGE_FILE_HEADER imgfileheader = pImgNtHdrs->FileHeader;
	PIMAGE_SECTION_HEADER pImgSectionHdr = (PIMAGE_SECTION_HEADER)(((PBYTE)pImgNtHdrs) + sizeof(IMAGE_NT_HEADERS));
	for (size_t i = 0; i < pImgNtHdrs->FileHeader.NumberOfSections; i++) {
		if (pImgSectionHdr->VirtualAddress == NULL) {
			continue;
		}
		if (pImgSectionHdr->Characteristics & IMAGE_SCN_MEM_EXECUTE && pImgSectionHdr->Characteristics & IMAGE_SCN_MEM_WRITE) {

			printf("[#] File Location: %ls\n", dllpath);
			printf("[#] SectionName: %s , with [RWX] permission\n", (CHAR*)pImgSectionHdr->Name);
			printf("\n\n");
		}

		pImgSectionHdr = (PIMAGE_SECTION_HEADER)((PBYTE)pImgSectionHdr + (DWORD)sizeof(IMAGE_SECTION_HEADER));
	}
	*pBuff = NULL;

_EndOfFunction:
	if (hFile) {
		CloseHandle(hFile);
	}
	return;	
}

void enumerate_files(wchar_t* folder) {
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = NULL;
	wchar_t sPath[MAX_PATH];
	wchar_t tempath[MAX_PATH*2];

	swprintf(sPath, sizeof(sPath), L"%ls\\*", folder);
	hFind = FindFirstFileW(sPath, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return;
	}
	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
			continue;
		}
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			swprintf(tempath, sizeof(tempath), L"%ls\\%ls\\", folder, ffd.cFileName);
			enumerate_files(tempath);
		}
		else
		{
			if (!wcscmp(&ffd.cFileName[wcslen(ffd.cFileName) - 4], L".dll")){
				swprintf(tempath, sizeof(tempath), L"%ls\\%ls", folder, ffd.cFileName);
				parsePESection(tempath);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

int main(int argc, char* argv[]) {
	wchar_t path[MAX_PATH];
	if (argc != 2) {
		printf("[+] Please privide the folder path ex : rwxfinder.exe C:\\");
		return 0;
	}
	swprintf_s(path, MAX_PATH, L"%hs", argv[1]);
	printf("Starting path: %ls\n",path);
	enumerate_files(path);
	printf("[+] Done. ");
	return 0;
}

