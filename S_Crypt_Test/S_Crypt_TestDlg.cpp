// S_Crypt_TestDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "S_Crypt_Test.h"
#include "S_Crypt_TestDlg.h"
#include <WinSvc.h>
#include <string>
#include <atlbase.h>
using namespace std;

typedef int (__stdcall* PStartProtect)();
typedef int (__stdcall* PStopProtect)();
typedef int (__stdcall* PCAddSecretProc)(CString ProcName);
typedef int (__stdcall* PDelSecretProc)();
PStartProtect StartProtect = NULL;
PStopProtect StopProtect = NULL;
PCAddSecretProc AddSecretProc = NULL;
PDelSecretProc DelSecretProc = NULL;

HMODULE hDllLib = NULL;
bool StartedOfDriver = false;
bool StartedOfProtect = false;
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


// CS_Crypt_TestDlg �Ի���
CS_Crypt_TestDlg::CS_Crypt_TestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CS_Crypt_TestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CS_Crypt_TestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, ProcList);
	DDX_Control(pDX, IDC_COMBO1, ProcessName);
}

BEGIN_MESSAGE_MAP(CS_Crypt_TestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	
		ON_BN_CLICKED(IDC_BUTTON2, &CS_Crypt_TestDlg::OnBnStartDriver)
		ON_BN_CLICKED(IDC_BUTTON7, &CS_Crypt_TestDlg::OnBnStopDriver)
		//ON_BN_CLICKED(IDC_BUTTON1, &CS_Crypt_TestDlg::OnBnStartProtect)
		//ON_BN_CLICKED(IDC_BUTTON3, &CS_Crypt_TestDlg::OnBnStopProtect)
		ON_BN_CLICKED(IDC_BUTTON4, &CS_Crypt_TestDlg::OnBnAddSecretProc)
		//ON_BN_CLICKED(IDC_BUTTON5, &CS_Crypt_TestDlg::OnBnDelSecretProc)
		ON_WM_CLOSE()
END_MESSAGE_MAP()


// CS_Crypt_TestDlg ��Ϣ�������

BOOL CS_Crypt_TestDlg::OnInitDialog()
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
	if(hDllLib == NULL)
		hDllLib = LoadLibrary("S_Crypt_Dll.dll");
	if (hDllLib)
	{
		StartProtect = (PStartProtect)GetProcAddress(hDllLib, "StartProtect");
		StopProtect = (PStopProtect)GetProcAddress(hDllLib, "StopProtect");
		AddSecretProc = (PCAddSecretProc)GetProcAddress(hDllLib, "AddSecretProc");
		DelSecretProc = (PDelSecretProc)GetProcAddress(hDllLib, "DelSecretProc");	

		if((StartProtect == NULL)||(StopProtect == NULL)||(AddSecretProc == NULL)||(DelSecretProc == NULL))
		{
			MessageBox("Function Address is Failed");
			ExitProcess(0);
		}
	}
	

	ProcessName.InsertString(0,"winword.exe");
	ProcessName.InsertString(1,"excel.exe");
	ProcessName.InsertString(2,"powerpnt.exe");
	ProcessName.InsertString(3,"wps.exe");	
	ProcessName.InsertString(4,"et.exe");	
	ProcessName.InsertString(5,"wpp.exe");	
	ProcessName.InsertString(6,"wpscloudsvr.exe");
	ProcessName.InsertString(7,"notepad.exe");
	ProcessName.InsertString(8,"photoshop.exe"); 
	ProcessName.InsertString(9,"visio.exe");	
	
	
	CRect rc;//������Ͽ�ĸ�
	ProcessName.GetWindowRect(&rc);   
	ScreenToClient(&rc);   
	ProcessName.MoveWindow(rc.left,rc.top,rc.Width(),rc.Height()*10);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CS_Crypt_TestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CS_Crypt_TestDlg::OnPaint()
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
HCURSOR CS_Crypt_TestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



// void CS_Crypt_TestDlg::OnBnStartProtect()
// {
// 	// TODO: �ڴ���ӿؼ�֪ͨ����������
// 	if (StartedOfDriver == false)
// 	{
// 		MessageBox("Please Firt StartDriver");
// 		return;
// 	}
// 	if (StartedOfProtect == true)
// 	{
// 		MessageBox("Protect alread Start");
// 		return;
// 	}
// 	StartProtect();
// 	StartedOfProtect = true;
// 	MessageBox("Protect is open!");
// 
// }
// 
// void CS_Crypt_TestDlg::OnBnStopProtect()
// {
// 	// TODO: �ڴ���ӿؼ�֪ͨ����������
// 	if (StartedOfDriver == false)
// 	{
// 		MessageBox("Please Firt StartDriver");
// 		return;
// 	}
// 	if (StartedOfProtect == false)
// 	{
// 		MessageBox("Protect alread Stop");
// 		return;
// 	}
// 	DelSecretProc();
// 	StopProtect();
// 	StartedOfProtect = false;
// 	ProcList.ResetContent();	
// 	MessageBox("Protect is off!");
// }

void CS_Crypt_TestDlg::OnBnAddSecretProc()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (StartedOfDriver == false)
	{
		MessageBox("Please Firt StartDriver");
		return;
	}

// 	if (StartedOfProtect == false)
// 	{
// 		MessageBox("Please Firt StartProtect");
// 		return;
// 	}

	CString temp;
	ProcessName.GetWindowText(temp);
	if (!strlen(temp))
	{
		MessageBox("Please select a item from Proclist!");
	}else
	{		
		AddSecretProc(temp);
		ProcList.AddString(temp); 
		MessageBox("Addition is ok !");
	}
}

/*
void CS_Crypt_TestDlg::OnBnDelSecretProc()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (StartedOfDriver == false)
	{
		MessageBox("Please Firt StartDriver");
		return;
	}

// 	if (StartedOfProtect == false)
// 	{
// 		MessageBox("Please Firt StartProtect");
// 		return;
// 	}
	DelSecretProc();
	ProcList.ResetContent();		
}
*/

void CS_Crypt_TestDlg::OnBnStartDriver()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	SC_HANDLE	schSCManager;
	BOOL ret;
	SC_HANDLE  schService;
	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	schService = OpenService(schSCManager,"S_Crypt",SC_MANAGER_ALL_ACCESS);
	if (!schService)
	{
		MessageBox("Please Run By Administrator");
		return;
	}

	SERVICE_STATUS state;
	QueryServiceStatus(schService,&state);
	if (state.dwCurrentState == SERVICE_RUNNING)
	{
		MessageBox("Driver alread Start!");
	
		CloseServiceHandle( schService );
		CloseServiceHandle(schSCManager);
		StartedOfDriver = true;
		return;
	}

	ret = StartService(schService, 0, NULL );
	if (!ret)
	{
		MessageBox("Driver Launch Falure!");
		return;
		
	}
	CloseServiceHandle( schService );
	CloseServiceHandle(schSCManager);
	StartedOfDriver = true;
	StartProtect();
	//AddSecretProc("wpscloudsvr.exe");
	//AddSecretProc("wpscenter.exe");
	MessageBox("Driver Launch Success!");

}


void CS_Crypt_TestDlg::OnBnStopDriver()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	ProcList.ResetContent();	

	SC_HANDLE	schSCManager;
	SC_HANDLE  schService;
	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	schService = OpenService(schSCManager,"S_Crypt",SC_MANAGER_ALL_ACCESS);
	if (!schService)
	{
		MessageBox("Please Run By Administrator");
		return;
	}

	SERVICE_STATUS state;
	QueryServiceStatus(schService,&state);
	if (state.dwCurrentState == SERVICE_STOPPED)
	{
		MessageBox("Driver alread Stop!");

		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		StartedOfDriver = false;
		return;
	}

	ControlService(schService,SERVICE_CONTROL_STOP, &state);
	CloseServiceHandle( schService );
	CloseServiceHandle(schSCManager);
	StartedOfDriver = false;	
	MessageBox("Driver Stop Success!");
}

void CS_Crypt_TestDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	if(StartedOfDriver == true)
	{
		SC_HANDLE	schSCManager;
		SC_HANDLE  schService;
		schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

		schService = OpenService(schSCManager,"S_Crypt",SC_MANAGER_ALL_ACCESS);
		if (!schService)
		{
			//MessageBox("Driver alread Stop!");
			return CDialog::OnClose();
		}

		SERVICE_STATUS state;
		QueryServiceStatus(schService,&state);
		if (state.dwCurrentState == SERVICE_STOPPED)
		{
			//MessageBox("Driver alread Stop!");

			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			StartedOfDriver = false;
			return CDialog::OnClose();
		}

		ControlService(schService,SERVICE_CONTROL_STOP, &state);
		CloseServiceHandle( schService );
		CloseServiceHandle(schSCManager);
		StartedOfDriver = false;
		StartedOfProtect = false;
		MessageBox("Driver Stop Success!");

	}

	

	CDialog::OnClose();
}
