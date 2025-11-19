#pragma once

#include "resource.h"

// CPDLCSettingsDialog dialog

class CPDLCSettingsDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CPDLCSettingsDialog)

public:
	CPDLCSettingsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPDLCSettingsDialog();

	CString m_Logon;
	CString m_Password;
	int m_Sound;

// Dialog Data
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSound();
	afx_msg void OnBnClickedOk();
};