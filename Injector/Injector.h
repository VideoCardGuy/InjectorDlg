// Injector.h : Injector DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CInjectorApp
// �йش���ʵ�ֵ���Ϣ������� Injector.cpp
//

class CInjectorApp : public CWinApp
{
public:
	CInjectorApp();

// ��д
public:
	virtual BOOL InitInstance();
	__declspec(dllexport) void BBB(DWORD dwID);
	DECLARE_MESSAGE_MAP()
};
