// RestoreToolDlg.h : ͷ�ļ�
//

#pragma once


// CRestoreToolDlg �Ի���
class CRestoreToolDlg : public CDialog
{
// ����
public:
	CRestoreToolDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RESTORETOOL_DIALOG };

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
	afx_msg void OnBnClickedOpenFile();
public:
	afx_msg void OnBnClickedSaveFile();
public:
	afx_msg void OnBnClickedRestore();
};
