// S_Crypt_TestDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CS_Crypt_TestDlg �Ի���
class CS_Crypt_TestDlg : public CDialog
{
// ����
public:
	CS_Crypt_TestDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_S_CRYPT_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
