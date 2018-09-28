// S_Crypt_TestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CS_Crypt_TestDlg 对话框
class CS_Crypt_TestDlg : public CDialog
{
// 构造
public:
	CS_Crypt_TestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_S_CRYPT_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnStartDriver();
public:
	afx_msg void OnBnStopDriver();
public:
	afx_msg void OnBnAddSecretProc();
//public:
	//afx_msg void OnBnDelSecretProc();
//public:
	//afx_msg void OnBnStartProtect();
//public:
	//afx_msg void OnBnStopProtect();

public:
	CListBox ProcList;

public:
	CComboBox ProcessName;

public:
	afx_msg void OnClose();
};
