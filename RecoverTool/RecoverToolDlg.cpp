// RecoverToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RecoverTool.h"
#include "RecoverToolDlg.h"

#include "ChainNode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef struct FileHead
{

	BYTE DataFlag[4];
	long FileTailOffset;
	int TailCount;
	int Reserved;

} DataHead;

void WriteFileTempFile(CHAR* filePath,CHAR* content, int size);
void File_DecryptBuffer(PVOID buffer,int Length);
void File_EncryptBuffer(PVOID buffer,int Length);

extern ChainNode* head;
CHAR TempFileBody[2048] = {0};
int TempFileBodyLen = 0;
int FileTailCount = 0;//每插入一个密文后就要加1，并在最后构造的时候写入到文件头的TailCount中
CHAR* FileTailTable = NULL;

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

// CRecoverToolDlg 对话框
CRecoverToolDlg::CRecoverToolDlg(CWnd* pParent /*=NULL*/)
: CDialog(CRecoverToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRecoverToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRecoverToolDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CRecoverToolDlg::OnBnDecryptTempFile)
	ON_BN_CLICKED(IDCANCEL, &CRecoverToolDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK3, &CRecoverToolDlg::OnBnBuildTempFile)
	ON_BN_CLICKED(IDC_BUTTON1, &CRecoverToolDlg::OnBnInsertCiphertext)
	ON_BN_CLICKED(IDC_BUTTON2, &CRecoverToolDlg::OnBnInsertCleartext)
END_MESSAGE_MAP()


// CRecoverToolDlg 消息处理程序

BOOL CRecoverToolDlg::OnInitDialog()
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

void CRecoverToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CRecoverToolDlg::OnPaint()
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
HCURSOR CRecoverToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//先根据文件尾来解析加密的部分，然后再砍掉文件头 & 文件尾
void CRecoverToolDlg::OnBnDecryptTempFile()
{
	// TODO: 在此添加控件通知处理程序代码
	//File_DecryptBuffer
	
}

void CRecoverToolDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	OnCancel();
}

//TempFile 最终文件需要根据内容长度+文件头长度+文件尾长度来分配空间，并将这三部分逐步写入进去。
void CRecoverToolDlg::OnBnBuildTempFile()
{
	// TODO: 在此添加控件通知处理程序代码
	// Build Encryption File to TempFile
	// WriteFile("c:\\TempFile", "1234\r\n56789", 11);  

	DataHead* Head_One = (DataHead*)malloc(sizeof(DataHead));
	memset(Head_One,0,sizeof(Head_One));

	Head_One->DataFlag[0]='~';
	Head_One->DataFlag[1]='!';
	Head_One->DataFlag[2]='@';
	Head_One->DataFlag[3]='#';

	int DataHeadLen = sizeof(DataHead);
	
	Head_One->TailCount = FileTailCount;
	Head_One->FileTailOffset = DataHeadLen + TempFileBodyLen;//写入加密表的偏移
	
	//先向最终文件写入文件头
	WriteFileTempFile("c:\\TempFile",(CHAR*)Head_One,DataHeadLen);

	//向最终文件写入文件体，其中文件体包括明文和密文。
	WriteFileTempFile("c:\\TempFile",(CHAR*)TempFileBody,TempFileBodyLen);

	//最后向最终文件的尾部写入加密表	
	if ( FileTailTable != NULL )
	{
		int TailTableLen = head->Index * sizeof(ChainNode);
		WriteFileTempFile("c:\\TempFile",(CHAR*)FileTailTable,TailTableLen);
	}
	
	
}

void CRecoverToolDlg::OnBnInsertCiphertext()
{
	// TODO: 在此添加控件通知处理程序代码

	CHAR TempBuffer[256] = {0};
	GetDlgItemText(IDC_EDIT1,TempBuffer,256);

	int CurBodyLength = (int)strlen(TempBuffer);//CurBodyLength 是每次密文的长度，而不是总的TempFileBodyLen长度

	File_EncryptBuffer(TempBuffer,CurBodyLength);
	
	//每次都要存放到最终文件的文件体TempFileBody中
	memcpy(TempFileBody+TempFileBodyLen,TempBuffer,CurBodyLength);
	TempFileBodyLen += CurBodyLength;

	//设置加密表并分配空间将节点插入到加密表中
	ChainNode* Example = NULL;
	Example = (ChainNode*)malloc(sizeof(ChainNode));// 分配表头节点的空间
	memset(Example,0,sizeof(ChainNode));

	Example->DataLength = CurBodyLength;//存入加密数据的长度
	//Example->EncryptStart = TempFileBody + TempFileBodyLen;//存入加密位置

	InsertNode(Example);

}

void CRecoverToolDlg::OnBnInsertCleartext()
{
	// TODO: 在此添加控件通知处理程序代码
}




///////////////////////////////// Functions ///////////////////////////////// ///////////////////////////////// ///////////////////////////////// ///////////////////////////////// 


#define ENC_BUFF_KEY_BIT 0x77

void File_EncryptBuffer(PVOID buffer,int Length)
{
	PCHAR buf;
	UINT64 i; 
	buf = (PCHAR)buffer;

	for(i=0;i<Length;i++)
	{
		buf[i] ^= ENC_BUFF_KEY_BIT;
	}
}


//先扫有没有标志位，若没有则不做处理直接，还得验证有时要是从数据中间传过来怎么办（跳过数据头）？
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


void WriteFileTempFile(CHAR* filePath,CHAR* content, int size)  
{  
	//创建文件  
	HANDLE hFile = CreateFile(filePath,     //创建文件的名称。  
		GENERIC_WRITE,          // 写和读文件。  
		0,                      // 不共享读写。  
		NULL,                   // 缺省安全属性。  
		OPEN_EXISTING,          // CREATE_ALWAYS  覆盖文件（不存在则创建）    OPEN_EXISTING 打开文件（不存在则报错）  
		FILE_ATTRIBUTE_NORMAL, // 一般的文件。         
		NULL);                 // 模板文件为空。  
	if (hFile == INVALID_HANDLE_VALUE)  
	{  
		OutputDebugString(TEXT("CreateFile fail!\r\n"));  
	}  
	//设置偏移量 到文件尾部 配合OPEN_EXISTING使用 可实现追加写入文件  
	SetFilePointer(hFile, NULL, NULL, FILE_END);//确保从文件尾部接着写入
	//写文件  
	//const int BUFSIZE = 8192;//如果缓冲区不够可增加  
	//char chBuffer[BUFSIZE];  
	//memcpy(chBuffer, content, size);//也可使用strcpy  
	DWORD dwWritenSize = 0;  
	BOOL bRet = WriteFile(hFile, content, size, &dwWritenSize, NULL);  
	if (bRet)  
	{  
		OutputDebugString(TEXT("WriteFile is ok\r\n"));  
	}  
	//刷新文件缓冲区  
	FlushFileBuffers(hFile);  
} 