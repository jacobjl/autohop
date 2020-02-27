#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <time.h>

class CProcess
{
private:

public:

	PROCESSENTRY32 pGame;
	HANDLE hProcess;

	DWORD dwEngine;
	DWORD dwOverlay;



	DWORD FindProcess(const char *ccName, PROCESSENTRY32 *pEntry)
	{
		PROCESSENTRY32 pEntry32;
		pEntry32.dwSize = sizeof(PROCESSENTRY32);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

		if (!Process32First(hSnapshot, &pEntry32))
		{
			CloseHandle(hSnapshot);
			return 0;
		}
		do
		{
			if (!_strcmpi(pEntry32.szExeFile, ccName))
			{
				memcpy((void *)pEntry, (void *)&pEntry32, sizeof(PROCESSENTRY32));
				CloseHandle(hSnapshot);
				return pEntry32.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pEntry32));
		CloseHandle(hSnapshot);

		return 0;
	}

	DWORD FindThread(DWORD dwProcess)
	{
		THREADENTRY32 tEntry32;
		tEntry32.dwSize = sizeof(THREADENTRY32);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

		if (!Thread32First(hSnapshot, &tEntry32))
		{
			CloseHandle(hSnapshot);
			return 0;
		}
		do
		{
			if (tEntry32.th32OwnerProcessID == dwProcess)
			{
				CloseHandle(hSnapshot);
				return tEntry32.th32ThreadID;
			}
		} while (Thread32Next(hSnapshot, &tEntry32));
		CloseHandle(hSnapshot);

		return 0;
	}

	DWORD GetModuleBase(LPSTR lpModuleName, DWORD dwProcessId)
	{
		MODULEENTRY32 lpModuleEntry = { 0 };
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
		if (!hSnapShot)	return NULL;
		lpModuleEntry.dwSize = sizeof(lpModuleEntry);
		BOOL bModule = Module32First(hSnapShot, &lpModuleEntry);
		while (bModule)
		{
			if (!strcmp(lpModuleEntry.szModule, lpModuleName))
			{
				CloseHandle(hSnapShot);
				return (DWORD)lpModuleEntry.modBaseAddr;
			}

			bModule = Module32Next(hSnapShot, &lpModuleEntry);
		}

		CloseHandle(hSnapShot);
		return NULL;
	}

	void SetDebugPrivilege()
	{
		HANDLE hProcess = GetCurrentProcess(), hToken;
		TOKEN_PRIVILEGES priv;
		LUID luid;

		OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);
		LookupPrivilegeValue(0, "seDebugPrivilege", &luid);
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Luid = luid;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, false, &priv, 0, 0, 0);
		CloseHandle(hToken);
		CloseHandle(hProcess);
	}

	void Initialize()
	{
		SetDebugPrivilege();
		while (!FindProcess("svencoop.exe", &pGame)) Sleep(10);
		while (!(FindThread(pGame.th32ProcessID))) Sleep(10);
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pGame.th32ProcessID);
		while (dwEngine == 0x0) dwEngine = GetModuleBase("hw.dll", pGame.th32ProcessID);
		while (dwOverlay == 0x0) dwOverlay = GetModuleBase("gameoverlayrenderer.dll", pGame.th32ProcessID);
	}
};

extern CProcess gProcess;
