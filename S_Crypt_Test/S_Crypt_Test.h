// S_Crypt_Test.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CS_Crypt_TestApp:
// �йش����ʵ�֣������ S_Crypt_Test.cpp
//

class CS_Crypt_TestApp : public CWinApp
{
public:
	CS_Crypt_TestApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CS_Crypt_TestApp theApp;