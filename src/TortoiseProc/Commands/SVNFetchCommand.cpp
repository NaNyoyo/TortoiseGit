// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit

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
#include "stdafx.h"
#include "SVNFetchCommand.h"

#include "SysProgressDlg.h"
#include "ProgressDlg.h"
#include "Git.h"
#include "LogDlg.h"
#include "FileDiffDlg.h"

bool SVNFetchCommand::Execute()
{
	CString cmd, out, err;
	cmd = L"git.exe config svn-remote.svn.fetch";

	if (!g_Git.Run(cmd, &out, &err, CP_UTF8))
	{
		int start = out.Find(L':');
		if( start >=0 )
			out=out.Mid(start);

		if(out.Left(5) == L":refs")
			out=out.Mid(6);

		start = 0;
		out = out.Tokenize(L"\n", start);
	}
	else
	{
		MessageBox(hwndExplorer, L"Found no SVN remote.", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}

	CGitHash upstreamOldHash;
	if (g_Git.GetHash(upstreamOldHash, out))
	{
		MessageBox(hwndExplorer, g_Git.GetGitLastErr(L"Could not get upstream hash."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	CProgressDlg progress;
	progress.m_GitCmd=L"git.exe svn fetch";

	CGitHash upstreamNewHash; // declare outside lambda, because it is captured by reference
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		if (g_Git.GetHash(upstreamNewHash, out))
		{
			MessageBox(hwndExplorer, g_Git.GetGitLastErr(L"Could not get upstream hash after fetching."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		if (upstreamOldHash == upstreamNewHash)
			return;

		postCmdList.emplace_back(IDI_DIFF, L"Fetched Diff", [&]
		{
			CLogDlg dlg;
			dlg.SetParams(CTGitPath(L""), CTGitPath(L""), L"", upstreamOldHash.ToString() + L".." + upstreamNewHash.ToString(), 0);
			dlg.DoModal();
		});

		postCmdList.emplace_back(IDI_LOG, L"Fetched Log", [&]
		{
			CFileDiffDlg dlg;
			dlg.SetDiff(nullptr, upstreamNewHash.ToString(), upstreamOldHash.ToString());
			dlg.DoModal();
		});
	};

	return progress.DoModal() == IDOK;
}
