// AfxFind.h : AFXFIND �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C���ł��B
//

#if !defined(AFX_AFXFIND_H_INCLUDED_)
#define AFX_AFXFIND_H_INCLUDED_v

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"		// ���C�� �V���{��

enum AfxFind_WindowMessages {
	WM_CURRENT_FINDDIR	= WM_USER+1,
	WM_FINDDIR,
};

typedef enum {
	AFX_ERR_NO = -1,
	AFX_ERR_OPEN_FILE,
	AFX_ERR_NO_PARAM,
	AFX_ERR_FIND,
} AFX_ERR_T;

class AfxFind
{
public:
	static AfxFind* GetInstance();
	static void DeleteInstance();

	AfxFind();

	int FindThreadCallBack();

	void SetProgressWindowsHandle(HWND hWnd);
	HWND GetProgressWindowsHandle();

protected:
	FILE* m_ofp;
	static AfxFind* m_instance;
	HWND m_hWndProgress;
	
	void OutputMenu(wchar_t* lpszPath);
	bool FindFile(LPCTSTR lpszPath, LPCTSTR lpszFile, HWND hWnd);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif
