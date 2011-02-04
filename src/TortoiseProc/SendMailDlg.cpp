// SendMailDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SendMailDlg.h"
#include "MessageBox.h"
#include "commonresource.h"
#include "AppUtils.h"
#include "PatchListCtrl.h"
// CSendMailDlg dialog

IMPLEMENT_DYNAMIC(CSendMailDlg, CResizableStandAloneDialog)

CSendMailDlg::CSendMailDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSendMailDlg::IDD, pParent)
	, m_To(_T(""))
	, m_CC(_T(""))
	, m_Subject(_T(""))
	
	, m_regAttach(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Attach"),0)
	, m_regCombine(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Combine"),0)
{
	m_bAttachment  = m_regAttach;
	m_bCombine =     m_regCombine;
	this->m_ctrlList.m_ContextMenuMask &=~ m_ctrlList.GetMenuMask(CPatchListCtrl::MENU_SENDMAIL);
}

CSendMailDlg::~CSendMailDlg()
{
}

void CSendMailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SENDMAIL_TO, m_To);
	DDX_Text(pDX, IDC_SENDMAIL_CC, m_CC);
	DDX_Check(pDX, IDC_SENDMAIL_ATTACHMENT, m_bAttachment);
	DDX_Check(pDX, IDC_SENDMAIL_COMBINE, m_bCombine);
	DDX_Check(pDX, IDC_SENDMAIL_MAPI, m_bUseMAPI);
	DDX_Control(pDX, IDC_SENDMAIL_PATCHS, m_ctrlList);
	DDX_Control(pDX,IDC_SENDMAIL_SETUP, this->m_SmtpSetup);
	DDX_Control(pDX,IDC_SENDMAIL_TO,m_ctrlTO);
	DDX_Control(pDX,IDC_SENDMAIL_CC,m_ctrlCC);
}


BEGIN_MESSAGE_MAP(CSendMailDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SENDMAIL_COMBINE, &CSendMailDlg::OnBnClickedSendmailCombine)
	ON_BN_CLICKED(IDOK, &CSendMailDlg::OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SENDMAIL_PATCHS, &CSendMailDlg::OnLvnItemchangedSendmailPatchs)
	ON_NOTIFY(NM_DBLCLK, IDC_SENDMAIL_PATCHS, &CSendMailDlg::OnNMDblclkSendmailPatchs)
	ON_EN_CHANGE(IDC_SENDMAIL_SUBJECT, &CSendMailDlg::OnEnChangeSendmailSubject)
	ON_BN_CLICKED(IDC_SENDMAIL_MAPI, &CSendMailDlg::OnBnClickedSendmailMapi)
END_MESSAGE_MAP()


// CSendMailDlg message handlers

BOOL CSendMailDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_SENDMAIL_GROUP,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_TO,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_CC,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SUBJECT,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SETUP,TOP_RIGHT);

	AddAnchor(IDC_SENDMAIL_PATCHS,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	this->AddOthersToAnchor();
	EnableSaveRestore(_T("SendMailDlg"));

	m_ctrlCC.Init();
	m_ctrlTO.Init();

	m_ctrlCC.SetSeparator(_T(";"));
	m_ctrlTO.SetSeparator(_T(";"));

	m_AddressReg.SetMaxHistoryItems(0xFFFF);

	m_AddressReg.Load(_T("Software\\TortoiseGit\\TortoiseProc\\EmailAddress\\"),_T("email"));
	for(int i=0;i<m_AddressReg.GetCount();i++)
	{
		m_ctrlCC.AddSearchString(m_AddressReg.GetEntry(i));
		m_ctrlTO.AddSearchString(m_AddressReg.GetEntry(i));
	}

	m_ctrlList.SetExtendedStyle( m_ctrlList.GetExtendedStyle()| LVS_EX_CHECKBOXES );

	for(int i=0;i<m_PathList.GetCount();i++)
	{
		m_ctrlList.InsertItem(i,m_PathList[i].GetWinPathString());
		m_ctrlList.SetCheck(i,true);
	}
	
//	m_ctrlCC.AddSearchString(_T("Tortoisegit-dev@google.com"));
//	m_ctrlTO.AddSearchString(_T("Tortoisegit-dev@google.com"));
	this->UpdateData(FALSE);
	OnBnClickedSendmailCombine();
	return TRUE;
}
void CSendMailDlg::OnBnClickedSendmailCombine()
{
	this->UpdateData();
	this->GetDlgItem(IDC_SENDMAIL_SUBJECT)->EnableWindow(this->m_bCombine);
	if(m_bCombine)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(this->m_Subject);

	UpdateSubject();
}

void CSendMailDlg::OnBnClickedOk()
{
	this->UpdateData();

	if(this->m_To.IsEmpty() && this->m_CC.IsEmpty())
	{
		CMessageBox::Show(NULL,IDS_ERR_ADDRESS_NO_EMPTY,IDS_APPNAME,MB_OK|MB_ICONERROR);
		return;
	}
	int start =0;
	CString Address;
	while(start>=0)
	{
		Address=this->m_CC.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}
	start =0;
	while(start>=0)
	{
		Address=this->m_To.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}	
	
	this->m_PathList.Clear();
	for(int i=0;i<m_ctrlList.GetItemCount();i++)
	{
		CTGitPath path;
		if(m_ctrlList.GetCheck(i))
		{
			path.SetFromWin(m_ctrlList.GetItemText(i,0));
			this->m_PathList.AddPath(path);
		}
	}

	m_regAttach=m_bAttachment;
	m_regCombine=m_bCombine;

	OnOK();
}

void CSendMailDlg::UpdateSubject()
{
	this->UpdateData();

	if(!this->m_bCombine)
	{
		if(m_ctrlList.GetSelectedCount()==1)
		{
			POSITION pos=m_ctrlList.GetFirstSelectedItemPosition();
			int index=m_ctrlList.GetNextSelectedItem(pos);
			if(this->m_MapPatch.find(index) == m_MapPatch.end() )
			{
				m_MapPatch[index].Parser(m_ctrlList.GetItemText(index,0));
			}
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(m_MapPatch[index].m_Subject);
		}
		else
		{
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(_T(""));
		}
	}
}

void CSendMailDlg::OnLvnItemchangedSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	UpdateSubject();
	
	*pResult = 0;
}

void CSendMailDlg::OnNMDblclkSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	CString path=this->m_ctrlList.GetItemText(pNMItemActivate->iItem,0);
	CTGitPath gitpath;
	gitpath.SetFromWin(path);
	
	CAppUtils::StartUnifiedDiffViewer(path,gitpath.GetFilename());

	*pResult = 0;
}

void CSendMailDlg::OnEnChangeSendmailSubject()
{
	this->UpdateData();
	if(this->m_bCombine)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->GetWindowText(this->m_Subject);
}

void CSendMailDlg::OnBnClickedSendmailMapi()
{
	this->UpdateData();
	GetDlgItem(IDC_SENDMAIL_CC)->EnableWindow(!m_bUseMAPI);
}
