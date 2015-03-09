// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
// Copyright (C) 2011-2013 Sven Strickroth <email@cs-ware.de>

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
#include "Patch.h"

CSendMailPatch::CSendMailPatch(CString &To, CString &CC, CString &subject, bool bAttachment, bool bCombine)
	: CSendMailCombineable(To, CC, subject, bAttachment, bCombine)
{
}

CSendMailPatch::~CSendMailPatch()
{
}

int CSendMailPatch::SendAsSingleMail(CTGitPath &path, CGitProgressList * instance)
{
	ASSERT(instance);

	CString pathfile(path.GetWinPathString());
	CPatch patch;
	if (patch.Parse(pathfile))
	{
		instance->ReportError(_T("Could not open/parse ") + pathfile);
		return -2;
	}

	CString body;
	CStringArray attachments;
	if (m_bAttachment)
		attachments.Add(pathfile);
	else
		body = patch.m_strBody;

	return SendMail(path, instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, patch.m_Subject, body, attachments);
}

int CSendMailPatch::SendAsCombinedMail(CTGitPathList &list, CGitProgressList * instance)
{
	ASSERT(instance);

	CStringArray attachments;
	CString body;
	for (int i = 0; i < list.GetCount(); ++i)
	{
		CPatch patch;
		if (patch.Parse((CString &)list[i].GetWinPathString()))
		{
			instance->ReportError(_T("Could not open/parse ") + list[i].GetWinPathString());
			return -2;
		}
		if (m_bAttachment)
		{
			attachments.Add(list[i].GetWinPathString());
			body += patch.m_Subject;
			body += _T("\r\n");
		}
		else
		{
			try
			{
				g_Git.StringAppend(&body, (BYTE*)patch.m_Body.GetBuffer(), CP_UTF8, patch.m_Body.GetLength());
			}
			catch (CMemoryException *)
			{
				instance->ReportError(_T("Out of memory. Could not parse ") + list[i].GetWinPathString());
				return -2;
			}
		}
	}
	return SendMail(CTGitPath(), instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, m_sSubject, body, attachments);
}

CPatch::CPatch()
{
}

CPatch::~CPatch()
{
}

int CPatch::Parse(CString &pathfile)
{
	CString str;

	m_PathFile = pathfile;

	CFile PatchFile;

	if (!PatchFile.Open(m_PathFile, CFile::modeRead))
		return -1;

	PatchFile.Read(m_Body.GetBuffer((UINT)PatchFile.GetLength()), (UINT)PatchFile.GetLength());
	m_Body.ReleaseBuffer();
	PatchFile.Close();

	try
	{
		int start=0;
		CStringA one;
		one=m_Body.Tokenize("\n",start);

		if (start == -1)
			return -1;
		one=m_Body.Tokenize("\n",start);
		if(one.GetLength()>6)
			g_Git.StringAppend(&m_Author, (BYTE*)one.GetBuffer() + 6, CP_UTF8, one.GetLength() - 6);

		if (start == -1)
			return -1;
		one=m_Body.Tokenize("\n",start);
		if(one.GetLength()>6)
			g_Git.StringAppend(&m_Date, (BYTE*)one.GetBuffer() + 6, CP_UTF8, one.GetLength() - 6);

		if (start == -1)
			return -1;
		one=m_Body.Tokenize("\n",start);
		if(one.GetLength()>9)
		{
			g_Git.StringAppend(&m_Subject, (BYTE*)one.GetBuffer() + 9, CP_UTF8, one.GetLength() - 9);
			while (m_Body.GetLength() > start && m_Body.GetAt(start) == _T(' '))
			{
				one = m_Body.Tokenize("\n", start);
				g_Git.StringAppend(&m_Subject, (BYTE*)one.GetBuffer(), CP_UTF8, one.GetLength());
			}
		}

		if (start + 1 < m_Body.GetLength())
			g_Git.StringAppend(&m_strBody, (BYTE*)m_Body.GetBuffer() + start + 1, CP_UTF8, m_Body.GetLength() - start - 1);
	}
	catch (CException *)
	{
		return -1;
	}

	return 0;
}
