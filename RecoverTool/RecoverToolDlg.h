// RecoverToolDlg.h : 头文件
//

#pragma once


// CRecoverToolDlg 对话框
class CRecoverToolDlg : public CDialog
{
// 构造
public:
	CRecoverToolDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_RECOVERTOOL_DIALOG };

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
	afx_msg void OnBnDecryptTempFile();
public:
	afx_msg void OnBnClickedCancel();
public:
	afx_msg void OnBnBuildTempFile();
public:
	afx_msg void OnBnInsertCiphertext();
public:
	afx_msg void OnBnInsertCleartext();
};
