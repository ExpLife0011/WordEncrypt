// RecoverTool.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CRecoverToolApp:
// �йش����ʵ�֣������ RecoverTool.cpp
//

class CRecoverToolApp : public CWinApp
{
public:
	CRecoverToolApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CRecoverToolApp theApp;