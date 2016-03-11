#include "stdafx.h"
//#include "worker.h"

HWND hWndListView;
BOOL bAscending = FALSE;

BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
	)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TCHAR szOutput[512];
	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"%s: %u\n", L"LookupPrivilegeValue error:", GetLastError());
		OutputDebugString(szOutput);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.
	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"%s: %u\n", L"AdjustTokenPrivileges error:", GetLastError());
		OutputDebugString(szOutput);
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"The token does not have the specified privilege. \n");
		OutputDebugString(szOutput);
		return FALSE;
	}
	return TRUE;
}

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if (lParamSort == 0) // column 0
	{
		if (bAscending)
		{
			if (lParam1 < lParam2)
			{
				return -1;
			}
			if (lParam1 > lParam2)
			{
				return 1;
			}
		}
		if (lParam1 < lParam2)
		{
			return 1;
		}
		if (lParam1 > lParam2)
		{
			return -1;
		}
		//OutputDebugString(L"");
	}
	return 0;
}

typedef enum
{
	SHOW_NO_ARROW,
	SHOW_UP_ARROW,
	SHOW_DOWN_ARROW
} SHOW_ARROW;

/*
* Description: Given a list - view control, this function will set the
*              sort arrow for a specific column.
* Requirements : ComCtl32.dll Version 6 (available on XP and greater).
*               To use Comctl32.dll version 6, you must specify it in a manifest.
*               You also need to add : #define _WIN32_WINNT 0x501
* before including commctrl.h
* Example : xListViewSetSortArrow(hListView, 0, SHOW_UP_ARROW);
*/
BOOL xListViewSetSortArrow(HWND hListView, INT idxColumn, SHOW_ARROW showArrow)
{
	HWND    hHeader = NULL;
	HDITEM  hdrItem = { 0 };

	hHeader = ListView_GetHeader(hListView);

	if (hHeader)
	{
		hdrItem.mask = HDI_FORMAT;
		if (Header_GetItem(hHeader, idxColumn, &hdrItem))
		{
			switch (showArrow)
			{
			case SHOW_UP_ARROW:
				hdrItem.fmt = (hdrItem.fmt & ~HDF_SORTDOWN) | HDF_SORTUP;
				break;
			case SHOW_DOWN_ARROW:
				hdrItem.fmt = (hdrItem.fmt & ~HDF_SORTUP) | HDF_SORTDOWN;
				break;
			default:
				break;
			}
			return Header_SetItem(hHeader, idxColumn, &hdrItem);
		}
	}
	return FALSE;
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL IsWow64(HANDLE hProcess)
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(hProcess, &bIsWow64))
		{
			//handle error
		}
	}
	return bIsWow64;
}


int DoMainWork()
{
	// Enable SeDebugPrivilege to be able to query other processes' virtual memory
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	SetPrivilege(hToken, L"SeDebugPrivilege", TRUE);

	// Enumerate running processes and query their memory
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32); // needed
	if (!Process32First(hSnapshot, &pe32))
	{
		OutputDebugStringA("No running processes??\n");
		return 1;
	}
	LVITEM lvI;
	memset(&lvI, 0, sizeof(LVITEM)); // needed, otherwise lvI's members are not valid because of stack's old values (garbage)
	lvI.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	MEMORY_BASIC_INFORMATION mbi;
	do
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
		if (hProcess != 0) // if able to query this process' memory
		{
			LPCVOID pRegionBase = (LPCVOID)0x10000; // start scanning from user-mode VM partition 
			while (VirtualQueryEx(hProcess, pRegionBase,
				(PMEMORY_BASIC_INFORMATION)&mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
			{
				if ((mbi.Type == MEM_PRIVATE)
					&& (mbi.Protect == PAGE_EXECUTE_READWRITE || mbi.Protect == PAGE_EXECUTE_READ))
				{
					// always insert a new item at index 0
					lvI.lParam = pe32.th32ProcessID;
					lvI.iSubItem = 0;
					lvI.pszText = new TCHAR[256];
					StringCchPrintf(lvI.pszText, sizeof(TCHAR) * 256 / sizeof(TCHAR), L"%d", pe32.th32ProcessID);
					ListView_InsertItem(hWndListView, &lvI);
					//ListView_SetItem(hWndListView, &lvI);
					ListView_SetItemText(hWndListView, 0, 1, pe32.szExeFile);
					break; // found a region with ER/EWR attribute, stop scanning this process' VM
				}
				else
				{
					pRegionBase = (PBYTE)pRegionBase + mbi.RegionSize; // not found a region with ER/EWR attribute yet, check the next region
					BOOL bIsWow64 = IsWow64(hProcess);
					if (bIsWow64 && (pRegionBase >= (LPCVOID)0x7ffeffff))
					{
						break; // reached the end of valid virtual memory partition for 32bit VM, stop scanning this process' VM
					}
				}
			}
		}
		CloseHandle(hProcess);
	} while (Process32Next(hSnapshot, &pe32));
	CloseHandle(hSnapshot);
	bAscending = TRUE;
	ListView_SortItems(hWndListView, CompareFunc, 0); // sorting ascending column 0;
	xListViewSetSortArrow(hWndListView, 0, bAscending ? SHOW_DOWN_ARROW : SHOW_UP_ARROW);

	//DWORD dwProcessIds[256];
	//DWORD dwSize = sizeof(dwProcessIds);
	//DWORD dwBytesReturned;
	//if (!EnumProcesses(dwProcessIds, dwSize, &dwBytesReturned))
	//{
	//	OutputDebugStringA("Failed to enumerate processes\n");
	//	break;
	//}
	//DWORD dwNumOfProcesses = dwBytesReturned / sizeof(DWORD);
	//OutputDebugStringA("");
	//for (DWORD i = 0; i < dwNumOfProcesses; i++)
	//{
	//	if (dwProcessIds[i] != 0 && dwProcessIds[i] != 4)
	//	{
	//		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, 19872);// dwProcessIds[i]);
	//		if (hProcess != 0)
	//		{
	//			MEMORY_BASIC_INFORMATION mbi;
	//			LPVOID pRegionBase = (LPVOID)0x1000;
	//			WCHAR szFileName[256];
	//			WCHAR szOutput[256];
	//			for (;;)
	//			{
	//				if (VirtualQueryEx(hProcess, pRegionBase,
	//					(PMEMORY_BASIC_INFORMATION)&mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
	//				{
	//					if ((mbi.Type == MEM_PRIVATE)
	//						&& (mbi.Protect == PAGE_EXECUTE_READWRITE || mbi.Protect == PAGE_EXECUTE_READ))
	//					{
	//						int iItem = ListView_InsertItem(hWndListView, &lvI);
	//						lvI.iItem = iItem;
	//						lvI.iSubItem = 0;
	//						lvI.lParam = dwProcessIds[i];
	//						StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(WCHAR), L"%d", dwProcessIds[i]);
	//						lvI.pszText = szOutput;
	//						ListView_SetItem(hWndListView, &lvI);
	//						//ListView_SetItemText(hWndListView, iItem, 0, szOutput);
	//						GetProcessImageFileName(hProcess, szFileName, sizeof(szFileName) / sizeof(WCHAR));
	//						StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(WCHAR), L"%s", PathFindFileName(szFileName));
	//						ListView_SetItemText(hWndListView, iItem, 1, szOutput);
	//						CloseHandle(hProcess); // found the first page with ER or ERW protection attribute. Stop scanning this process's virtual memory
	//						break;
	//					}
	//				}
	//				else // reached the end of the process's virtual memory (within user partition). Stop scanning this process's virtual memory
	//				{
	//					CloseHandle(hProcess);
	//					break;
	//				}
	//				pRegionBase = (PBYTE)pRegionBase + mbi.RegionSize;
	//			}
	//			//CloseHandle(hProcess);
	//		}
	//	}
	//}
	//bAscending = TRUE;
	//ListView_SortItems(hWndListView, CompareFunc, 0); // sorting ascending column 0;
	//xListViewSetSortArrow(hWndListView, 0, bAscending ? SHOW_DOWN_ARROW : SHOW_UP_ARROW);
	//OutputDebugStringA("");
	return 0;
}

void SortColumn(HWND hWndListView, int iCol)
{
	bAscending = !bAscending;
	ListView_SortItems(hWndListView, CompareFunc, iCol);
	xListViewSetSortArrow(hWndListView, 0, bAscending ? SHOW_DOWN_ARROW : SHOW_UP_ARROW);
}
