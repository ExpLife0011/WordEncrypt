// RecoverToolDlg.h : ͷ�ļ�
//

#pragma once


// CRecoverToolDlg �Ի���
class CRecoverToolDlg : public CDialog
{
// ����
public:
	CRecoverToolDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RECOVERTOOL_DIALOG };

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
