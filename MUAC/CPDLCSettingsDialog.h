#pragma once

#include "resource.h"
#ifndef __AFXWIN_H__
#include <afxwin.h>      // CWnd, CDialogEx
#endif

#ifndef __AFXDLGS_H__
#include <afxdialogex.h> // CDialogEx
#endif


// CCPDLCSettingsDialog dialog

class CCPDLCSettingsDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CCPDLCSettingsDialog)

public:
	CCPDLCSettingsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCPDLCSettingsDialog();

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
