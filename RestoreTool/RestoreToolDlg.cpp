// RestoreToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RestoreTool.h"
#include "RestoreToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
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


// CRestoreToolDlg 对话框




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


// CRestoreToolDlg 消息处理程序

BOOL CRestoreToolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRestoreToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
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
	CString  csFileExtName = csFileFullName.Right(csFileFullName.GetLength() - nPos); // 获取扩展名  
	return csFileExtName;  
} 

void CRestoreToolDlg::OnBnClickedOpenFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlgOpen(TRUE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
		NULL, 
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		(LPCTSTR)_TEXT("All Files (*.*)|*.*||"),
		NULL);
	if(dlgOpen.DoModal()==IDOK)
	{
		FilePathNameOpen=dlgOpen.GetPathName(); //文件名保存在了FilePathName里
	}else
		return;

	GetDlgItem(IDC_STATIC)->SetWindowText(FilePathNameOpen);
}

void CRestoreToolDlg::OnBnClickedSaveFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlgSave(FALSE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
		NULL, 
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		(LPCTSTR)_TEXT("All Files (*.*)|*.*||"),
		NULL);
	if(dlgSave.DoModal()==IDOK)
	{
		FilePathNameSave=dlgSave.GetPathName(); //文件名保存在了FilePathName里
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
		// TODO: 在此添加控件通知处理程序代码
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
		HANDLE hFileBuild = CreateFile(FilePathNameSave,     //创建文件的名称。  
			GENERIC_WRITE,          // 写和读文件。  
			0,                      // 不共享读写。  
			NULL,                   // 缺省安全属性。  
			CREATE_ALWAYS,          // CREATE_ALWAYS  覆盖文件（不存在则创建）    OPEN_EXISTING 打开文件（不存在则报错）  
			FILE_ATTRIBUTE_NORMAL, // 一般的文件。         
			NULL);                 // 模板文件为空。  
		if (hFileBuild == INVALID_HANDLE_VALUE)  
		{  
			MessageBox(L"Restore is failed !");
			return;
		}
		DWORD dwWritenSize = 0;  
		BOOL bRet = WriteFile(hFileBuild, FileBuffer, FileCount, &dwWritenSize, NULL);  
		if (bRet)  
		{  
			OutputDebugString(TEXT("WriteFile 写文件成功\r\n"));  
		}  
		//刷新文件缓冲区  
		FlushFileBuffers(hFile);
		CloseHandle(hFileBuild);
		MessageBox(L"Restore is ok !");
	}	
}