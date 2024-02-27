
// template_testDlg.h : header file
//

#pragma once


// CtemplatetestDlg dialog
class CtemplatetestDlg : public CDialogEx
{
// Construction
public:
	CtemplatetestDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TEMPLATE_TEST_DIALOG };
#endif

	BOOL  GetDevices();


    protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	HANDLE m_h;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	CString m_devsVal;
	CComboBox m_devsCtl;
	CString m_strSel;
	afx_msg void OnBnClickedTestlock();
	afx_msg void OnBnClickedTestlockforce();
	afx_msg void OnBnClickedTestunlock();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedOpen();
};


