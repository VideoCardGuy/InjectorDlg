// Injector.cpp : ���� DLL �ĳ�ʼ�����̡�
//

#include "stdafx.h"
#include "Injector.h"
#include <MyTools/Log.h>
#include "MyTools/CLExpression.h"
#include <MyTools/CmdLog.h>
//#include "Game.h"

#define _SELF L"Injector.cpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//Game * MyGame = NULL;
#define WM_MYMSG  WM_USER+100
//
//TODO: ����� DLL ����� MFC DLL �Ƕ�̬���ӵģ�
//		��Ӵ� DLL �������κε���
//		MFC �ĺ������뽫 AFX_MANAGE_STATE ����ӵ�
//		�ú�������ǰ�档
//
//		����:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// �˴�Ϊ��ͨ������
//		}
//
//		�˺������κ� MFC ����
//		������ÿ��������ʮ����Ҫ������ζ��
//		��������Ϊ�����еĵ�һ�����
//		���֣������������ж������������
//		������Ϊ���ǵĹ��캯���������� MFC
//		DLL ���á�
//
//		�й�������ϸ��Ϣ��
//		����� MFC ����˵�� 33 �� 58��
//

// CInjectorApp

BEGIN_MESSAGE_MAP(CInjectorApp, CWinApp)
END_MESSAGE_MAP()


// CInjectorApp ����

CInjectorApp::CInjectorApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CInjectorApp ����

CInjectorApp theApp;


// CInjectorApp ��ʼ��
/*
DWORD WINAPI MYThreadPro()
{
	if (MyGame == NULL && ::OpenMutexW(MUTEX_ALL_ACCESS,false,L"CL_MUTEX_DLL_INJECTOR") == NULL)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		MyGame = new Game();
		MyGame->DoModal();
	}

	return 0;
}*/

class CExpr : public MyTools::CExprFunBase, public MyTools::CClassInstance<CExpr>
{
public:
	CExpr() : _LockMapModule(L"_LockMapModule")
	{

	}
	virtual ~CExpr()
	{

	};

	virtual VOID Release()
	{

	}
	virtual std::vector<MyTools::ExpressionFunPtr>& GetVec()
	{
		static std::vector<MyTools::ExpressionFunPtr> Vec =
		{
			{ std::bind(&CExpr::Help, this, std::placeholders::_1), L"Help" },
			{ std::bind(&CExpr::InjectorDLL, this, std::placeholders::_1), L"InjectorDLL" },
		};
		return Vec;
	}
private:
	virtual VOID Help(CONST std::vector<std::wstring>&)
	{
		auto& Vec = GetVec();
		for (CONST auto& itm : Vec)
			LOG_C(MyTools::CLog::em_Log_Type::em_Log_Type_Custome, L"FunctionName=%s", itm.wsFunName.c_str());
	}

	VOID InjectorDLL(CONST std::vector<std::wstring>& VecParm)
	{
		if (VecParm.size() != 2)
		{
			LOG_C_E(L"InjectorDLL(Path,IsLoad)");
			return;
		}

		CONST std::wstring& wsPath = VecParm.at(0);
		BOOL IsLoad = VecParm.at(1) == L"1" ? TRUE : FALSE;

		if (IsLoad)
			Injector(wsPath);
		else
			FreeDLL(wsPath);
	}
private:
	BOOL Injector(_In_ CONST std::wstring& wsPath)
	{
		if (ExistDLL(wsPath))
		{
			LOG_C_D(L"Already Exist DLL:%s", wsPath.c_str());
			return FALSE;
		}

		HMODULE hmDLL = ::LoadLibraryW(wsPath.c_str());
		if (hmDLL == NULL)
		{
			LOG_C_E(L"Load DLL Faild:%s", wsPath.c_str());
			return FALSE;
		}

		_LockMapModule.Access([hmDLL, wsPath, this]
		{
			_MapModule.insert(std::make_pair(wsPath, hmDLL));
		});
		return TRUE;
	}

	BOOL FreeDLL(_In_ CONST std::wstring& wsPath)
	{
		if (!ExistDLL(wsPath))
		{
			LOG_C_D(L"UnExist DLL:%s", wsPath.c_str());
			return FALSE;
		}

		HMODULE hmDLL = NULL;
		_LockMapModule.Access([&hmDLL, this, wsPath]
		{
			auto itr = _MapModule.find(wsPath);
			if (itr != _MapModule.end())
			{
				hmDLL = itr->second;
				_MapModule.erase(itr);
			}
		});

		if (hmDLL != NULL)
		{
			while (::GetModuleHandle(wsPath.c_str()) != nullptr)
			{
				using fnExitDLL = VOID(WINAPIV*)(VOID);
				fnExitDLL ExitDLLPtr = reinterpret_cast<fnExitDLL>(::GetProcAddress(hmDLL, "ExitDLL"));
				if (ExitDLLPtr != nullptr)
				{
					LOG_C_D(L"Invoke ExitDLL");
					ExitDLLPtr();
				}

				LOG_C_D(L"Free DLL");
				::FreeLibrary(hmDLL);
			}
		}

		return TRUE;
	}

	BOOL ExistDLL(_In_ CONST std::wstring& wsPath) CONST
	{
		BOOL bExist = FALSE;
		_LockMapModule.Access([&bExist, this, wsPath]
		{
			bExist = _MapModule.find(wsPath) == _MapModule.end() ? FALSE : TRUE;
		});
		return bExist;
	}
private:
	std::map<std::wstring, HMODULE> _MapModule;
	MyTools::CLLock _LockMapModule;
};

HANDLE hWaitCmdThread = NULL;
DWORD WINAPI _WaitCmdThread(LPVOID)
{
	WCHAR wszSavePath[MAX_PATH] = { 0 };
	::GetCurrentDirectoryW(MAX_PATH, wszSavePath);
	::lstrcatW(wszSavePath, L"\\Log\\");
	MyTools::CLog::GetInstance().SetClientName(L"InjectorClient", wszSavePath);
	MyTools::CCmdLog::GetInstance().Run(L"InjectorClient", CExpr::GetInstance().GetVec());
	return 0;
}

BOOL CInjectorApp::InitInstance()
{
	CWinApp::InitInstance();
	//::CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)MYThreadPro,NULL,NULL,NULL); 
	if (hWaitCmdThread == NULL)
	{
		hWaitCmdThread = cbBEGINTHREADEX(NULL, NULL, _WaitCmdThread, NULL, NULL, NULL);
		::CloseHandle(hWaitCmdThread);
	}
	return TRUE;
}

__declspec(dllexport) void CInjectorApp::BBB(DWORD dwID)
{
}