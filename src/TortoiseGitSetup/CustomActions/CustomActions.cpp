// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2012, 2015-2016 - TortoiseGit
// Copyright (C) 2003-2008, 2012 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//


/* BIG FAT WARNING: Do not use any functions which require the C-Runtime library
   in this custom action dll! The runtimes might not be installed yet!
*/

#include "stdafx.h"
#include "resource.h"
#include <shlwapi.h>
#include <shellapi.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "shell32")

#define TGIT_CACHE_WINDOW_NAME _T("TGitCacheWindow")

BOOL APIENTRY DllMain( HANDLE /*hModule*/,
					   DWORD  /*ul_reason_for_call*/,
					   LPVOID /*lpReserved*/
					 )
{
	return TRUE;
}

UINT __stdcall TerminateCache(MSIHANDLE /*hModule*/)
{
	HWND hWnd = FindWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME);
	if (hWnd)
	{
		PostMessage(hWnd, WM_CLOSE, (WPARAM)nullptr, (LPARAM)nullptr);
		for (int i=0; i<10; ++i)
		{
			Sleep(500);
			if (!IsWindow(hWnd))
			{
				// Cache is gone!
				return ERROR_SUCCESS;
			}
		}
		// Don't return ERROR_FUNCTION_FAILED, because even if the cache is still
		// running, the installer will overwrite the file, and we require a
		// reboot anyway after upgrading.
		return ERROR_SUCCESS;
	}
	// cache wasn't even running
	return ERROR_SUCCESS;
}

UINT __stdcall OpenDonatePage(MSIHANDLE /*hModule*/)
{
	ShellExecute(nullptr, _T("open"), _T("https://tortoisegit.org/donate"), nullptr, nullptr, SW_SHOW);
	return ERROR_SUCCESS;
}

UINT __stdcall MsgBox(MSIHANDLE /*hModule*/)
{
	MessageBox(nullptr, _T("CustomAction \"MsgBox\" running"), _T("Installer"), MB_ICONINFORMATION);
	return ERROR_SUCCESS;
}

UINT __stdcall RestartExplorer(MSIHANDLE /*hModule*/)
{
	HMODULE hModule = nullptr;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(MsgBox), &hModule);

	if (!hModule)
		return ERROR_SUCCESS;

	HRSRC hRestartExplorerRes = FindResource(hModule, MAKEINTRESOURCE(IDR_RESTARTEXPLORER), RT_RCDATA);
	if (!hRestartExplorerRes)
		return ERROR_SUCCESS;

	HGLOBAL hRestartExplorerGlobal = LoadResource(hModule, hRestartExplorerRes);
	if (!hRestartExplorerGlobal)
		return ERROR_SUCCESS;

	TCHAR szTempPath[MAX_PATH];
	if (!GetTempPath(_countof(szTempPath) - 15, szTempPath))
		return ERROR_SUCCESS;

	TCHAR szTempFileName[MAX_PATH + 1];
	if (!GetTempFileName(szTempPath, L"REx", 0, szTempFileName))
		return ERROR_SUCCESS;

	size_t len = wcsnlen_s(szTempFileName, _countof(szTempPath));
	if (len < 14)
		return  ERROR_SUCCESS;
	szTempFileName[len - 3] = L'e';
	szTempFileName[len - 2] = L'x';
	szTempFileName[len - 1] = L'e';

	HANDLE hFile = CreateFile(szTempFileName, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return ERROR_SUCCESS;

	DWORD written = 0;
	if (!WriteFile(hFile, LockResource(hRestartExplorerGlobal), SizeofResource(hModule, hRestartExplorerRes), &written, nullptr) || written != SizeofResource(hModule, hRestartExplorerRes))
	{
		CloseHandle(hFile);
		return ERROR_SUCCESS;
	}

	CloseHandle(hFile);

	ShellExecute(nullptr, L"open", szTempFileName, nullptr, nullptr, SW_HIDE);

	return ERROR_SUCCESS;
}
