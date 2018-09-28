// RestoreToolDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RestoreTool.h"
#include "RestoreToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CRestoreToolDlg �Ի���




CRestoreToolDlg::CRestoreToolDlg(CWnd* pParent /*=NULL*/)
: CDialog(CRestoreToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRestoreToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRestoreToolDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CRestoreToolDlg::OnBnClickedOpenFile)
	ON_BN_CLICKED(IDC_BUTTON2, &CRestoreToolDlg::OnBnClickedSaveFile)
	ON_BN_CLICKED(IDC_BUTTON3, &CRestoreToolDlg::OnBnClickedRestore)
END_MESSAGE_MAP()


// CRestoreToolDlg ��Ϣ�������

BOOL CRestoreToolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CRestoreToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRestoreToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CRestoreToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#define ENC_BUFF_KEY_BIT 0x77
void File_DecryptBuffer(PVOID buffer,ULONG Length)
{
	PCHAR buf;
	DWORD i;

	buf = (PCHAR)buffer;

	for(i=0;i<Length;i++)
	{
		buf[i] ^= ENC_BUFF_KEY_BIT;
	}

}


CString FilePathNameOpen;
CString FilePathNameSave;

CString  GetFileExtName(CString csFileFullName)  
{  
	int nPos = csFileFullName.ReverseFind('.');  
	CString  csFileExtName = csFileFullName.Right(csFileFullName.GetLength() - nPos); // ��ȡ��չ��  
	return csFileExtName;  
} 

void CRestoreToolDlg::OnBnClickedOpenFile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CFileDialog dlgOpen(TRUE, //TRUEΪOPEN�Ի���FALSEΪSAVE AS�Ի���
		NULL, 
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		(LPCTSTR)_TEXT("All Files (*.*)|*.*||"),
		NULL);
	if(dlgOpen.DoModal()==IDOK)
	{
		FilePathNameOpen=dlgOpen.GetPathName(); //�ļ�����������FilePathName��
	}else
		return;

	GetDlgItem(IDC_STATIC)->SetWindowText(FilePathNameOpen);
}

void CRestoreToolDlg::OnBnClickedSaveFile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CFileDialog dlgSave(FALSE, //TRUEΪOPEN�Ի���FALSEΪSAVE AS�Ի���
		NULL, 
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		(LPCTSTR)_TEXT("All Files (*.*)|*.*||"),
		NULL);
	if(dlgSave.DoModal()==IDOK)
	{
		FilePathNameSave=dlgSave.GetPathName(); //�ļ�����������FilePathName��
	}else	
		return;

	CString FilePathNameOpenExt = GetFileExtName(FilePathNameOpen);
	FilePathNameSave = FilePathNameSave + FilePathNameOpenExt;
	GetDlgItem(IDC_STATIC1)->SetWindowText(FilePathNameSave);
}

void CRestoreToolDlg::OnBnClickedRestore()
{
	if (wcslen(FilePathNameOpen) == 0)
	{
		MessageBox(L"Please set path of OpenFile!");
	}else if (wcslen(FilePathNameSave) == 0)
	{
		MessageBox(L"Please set path of SaveFile!");
	}else
	{	
		// TODO: �ڴ���ӿؼ�֪ͨ����������
		HANDLE hFile = CreateFile(FilePathNameOpen,
			GENERIC_READ,
			NULL,
			NULL,
			OPEN_EXISTING,
			NULL,
			NULL
			);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			MessageBox(L"Restore is failed !");
			return;
		}
		int FileCount = GetFileSize(hFile,NULL);
		FileCount -= 0x1000;
		//FileCount -= 0x100;
		char* FileBuffer = new char[FileCount];
		memset(FileBuffer,0,FileCount);
		DWORD retdata = 0;
		SetFilePointer(hFile, 0x1000, NULL, FILE_BEGIN); 
		//SetFilePointer(hFile, 0x100, NULL, FILE_BEGIN);
		if (!ReadFile(hFile,FileBuffer,FileCount,&retdata,NULL))
		{
			MessageBox(L"Restore is failed !");
			return;
		}
		CloseHandle(hFile);
		File_DecryptBuffer(FileBuffer,FileCount);
		HANDLE hFileBuild = CreateFile(FilePathNameSave,     //�����ļ������ơ�  
			GENERIC_WRITE,          // д�Ͷ��ļ���  
			0,                      // �������д��  
			NULL,                   // ȱʡ��ȫ���ԡ�  
			CREATE_ALWAYS,          // CREATE_ALWAYS  �����ļ����������򴴽���    OPEN_EXISTING ���ļ����������򱨴�  
			FILE_ATTRIBUTE_NORMAL, // һ����ļ���         
			NULL);                 // ģ���ļ�Ϊ�ա�  
		if (hFileBuild == INVALID_HANDLE_VALUE)  
		{  
			MessageBox(L"Restore is failed !");
			return;
		}
		DWORD dwWritenSize = 0;  
		BOOL bRet = WriteFile(hFileBuild, FileBuffer, FileCount, &dwWritenSize, NULL);  
		if (bRet)  
		{  
			OutputDebugString(TEXT("WriteFile д�ļ��ɹ�\r\n"));  
		}  
		//ˢ���ļ�������  
		FlushFileBuffers(hFile);
		CloseHandle(hFileBuild);
		MessageBox(L"Restore is ok !");
	}	
}