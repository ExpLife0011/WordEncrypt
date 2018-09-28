// RecoverToolDlg.cpp : ʵ���ļ�
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
int FileTailCount = 0;//ÿ����һ�����ĺ��Ҫ��1������������ʱ��д�뵽�ļ�ͷ��TailCount��
CHAR* FileTailTable = NULL;

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

// CRecoverToolDlg �Ի���
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


// CRecoverToolDlg ��Ϣ�������

BOOL CRecoverToolDlg::OnInitDialog()
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRecoverToolDlg::OnPaint()
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
HCURSOR CRecoverToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//�ȸ����ļ�β���������ܵĲ��֣�Ȼ���ٿ����ļ�ͷ & �ļ�β
void CRecoverToolDlg::OnBnDecryptTempFile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//File_DecryptBuffer
	
}

void CRecoverToolDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnCancel();
}

//TempFile �����ļ���Ҫ�������ݳ���+�ļ�ͷ����+�ļ�β����������ռ䣬��������������д���ȥ��
void CRecoverToolDlg::OnBnBuildTempFile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	Head_One->FileTailOffset = DataHeadLen + TempFileBodyLen;//д����ܱ��ƫ��
	
	//���������ļ�д���ļ�ͷ
	WriteFileTempFile("c:\\TempFile",(CHAR*)Head_One,DataHeadLen);

	//�������ļ�д���ļ��壬�����ļ���������ĺ����ġ�
	WriteFileTempFile("c:\\TempFile",(CHAR*)TempFileBody,TempFileBodyLen);

	//����������ļ���β��д����ܱ�	
	if ( FileTailTable != NULL )
	{
		int TailTableLen = head->Index * sizeof(ChainNode);
		WriteFileTempFile("c:\\TempFile",(CHAR*)FileTailTable,TailTableLen);
	}
	
	
}

void CRecoverToolDlg::OnBnInsertCiphertext()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	CHAR TempBuffer[256] = {0};
	GetDlgItemText(IDC_EDIT1,TempBuffer,256);

	int CurBodyLength = (int)strlen(TempBuffer);//CurBodyLength ��ÿ�����ĵĳ��ȣ��������ܵ�TempFileBodyLen����

	File_EncryptBuffer(TempBuffer,CurBodyLength);
	
	//ÿ�ζ�Ҫ��ŵ������ļ����ļ���TempFileBody��
	memcpy(TempFileBody+TempFileBodyLen,TempBuffer,CurBodyLength);
	TempFileBodyLen += CurBodyLength;

	//���ü��ܱ�����ռ佫�ڵ���뵽���ܱ���
	ChainNode* Example = NULL;
	Example = (ChainNode*)malloc(sizeof(ChainNode));// �����ͷ�ڵ�Ŀռ�
	memset(Example,0,sizeof(ChainNode));

	Example->DataLength = CurBodyLength;//����������ݵĳ���
	//Example->EncryptStart = TempFileBody + TempFileBodyLen;//�������λ��

	InsertNode(Example);

}

void CRecoverToolDlg::OnBnInsertCleartext()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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


//��ɨ��û�б�־λ����û����������ֱ�ӣ�������֤��ʱҪ�Ǵ������м䴫������ô�죨��������ͷ����
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
	//�����ļ�  
	HANDLE hFile = CreateFile(filePath,     //�����ļ������ơ�  
		GENERIC_WRITE,          // д�Ͷ��ļ���  
		0,                      // �������д��  
		NULL,                   // ȱʡ��ȫ���ԡ�  
		OPEN_EXISTING,          // CREATE_ALWAYS  �����ļ����������򴴽���    OPEN_EXISTING ���ļ����������򱨴�  
		FILE_ATTRIBUTE_NORMAL, // һ����ļ���         
		NULL);                 // ģ���ļ�Ϊ�ա�  
	if (hFile == INVALID_HANDLE_VALUE)  
	{  
		OutputDebugString(TEXT("CreateFile fail!\r\n"));  
	}  
	//����ƫ���� ���ļ�β�� ���OPEN_EXISTINGʹ�� ��ʵ��׷��д���ļ�  
	SetFilePointer(hFile, NULL, NULL, FILE_END);//ȷ�����ļ�β������д��
	//д�ļ�  
	//const int BUFSIZE = 8192;//�������������������  
	//char chBuffer[BUFSIZE];  
	//memcpy(chBuffer, content, size);//Ҳ��ʹ��strcpy  
	DWORD dwWritenSize = 0;  
	BOOL bRet = WriteFile(hFile, content, size, &dwWritenSize, NULL);  
	if (bRet)  
	{  
		OutputDebugString(TEXT("WriteFile is ok\r\n"));  
	}  
	//ˢ���ļ�������  
	FlushFileBuffers(hFile);  
} 