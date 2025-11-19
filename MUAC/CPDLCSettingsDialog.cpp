// CPDLCSettingsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CPDLCSettingsDialog.hpp"
#include "afxdialogex.h"


// CPDLCSettingsDialog dialog

IMPLEMENT_DYNAMIC(CPDLCSettingsDialog, CDialogEx)

CPDLCSettingsDialog::CPDLCSettingsDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPDLCSettingsDialog::IDD, pParent)
	, m_Logon(_T("EGKK"))
	, m_Password(_T("PASSWORD"))
	, m_Sound(1)
{

}

CPDLCSettingsDialog::~CPDLCSettingsDialog()
{
}

void CPDLCSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGON, m_Logon);
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	DDX_Check(pDX, IDC_SOUND, m_Sound);
}


BEGIN_MESSAGE_MAP(CPDLCSettingsDialog, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPDLCSettingsDialog::OnBnClickedOk)
END_MESSAGE_MAP()

void CPDLCSettingsDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnOK();
}