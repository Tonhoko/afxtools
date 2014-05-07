/**
 * ����w�̃t�@�C������SVN�̕ύX�t�@�C���ꗗ��\������v���O�C��
 * 
 */
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <wchar.h>
#include "afxwapi.h"
#include "afxcom.h"
#include "version.h"

#define PLUGIN_NAME    L"afxsvn"
#define PLUGIN_AUTHOR  L"yuratomo"
#define MAX_LINE_SIZE       4096
#define MAX_LOG_ITEM_LEN    1024

typedef enum _Mode {
	Mode_Status,
} Mode;

typedef struct _StatusItem {
	wchar_t path[MAX_LINE_SIZE];
} StatusItem;

typedef struct _PluginData {
	wchar_t wszBaseDir[API_MAX_PATH_LENGTH];
	AfxwInfo afx;
	Mode mode;
	int status_count;
	int status_index;
	StatusItem* status_items;
} PluginData, *lpPluginData;

IDispatch* pAfxApp = NULL;
wchar_t _ini_path[MAX_PATH];
DWORD   _item_max = 64;
HINSTANCE _instance;

BOOL CreateSubDirectory(wchar_t *szPath);
void TrimCarriageReturn(wchar_t* wszBaseDir);
void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to);

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
			_instance = hinstDLL;
            setlocale( LC_ALL, "Japanese" );
            AfxInit(pAfxApp);
            break;

        case DLL_PROCESS_DETACH:
            AfxCleanup(pAfxApp);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return  TRUE;
}

/**
 * �v���O�C�������擾����B
 * @param[out] pluginInfo    �v���O�C�����B
 */
void WINAPI ApiGetPluginInfo(lpApiInfo pluginInfo)
{
	if (pluginInfo == NULL) {
		return;
	}

	memset(pluginInfo, 0, sizeof(ApiInfo));

	// �v���O�C����
	wcsncpy(pluginInfo->szPluginName, PLUGIN_NAME, API_MAX_PLUGIN_NAME_LENGTH-1);

	// ���
	wcsncpy(pluginInfo->szAuthorName, PLUGIN_AUTHOR, API_MAX_AUTHOR_NAME_LENGTH-1);

	// �v���O�C���o�[�W����
	pluginInfo->dwVersion = MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_BUGFIX, VERSION_REVISION);
}

DWORD run(wchar_t* cmdline, const wchar_t* redirectTo)
{
    STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
    wchar_t Args[4096];

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    Args[0] = 0;
    wcscat(Args, cmdline);  
    wcscat(Args, L" > \""); 
    wcscat(Args, redirectTo);
    wcscat(Args, L"\" 2>&1"); 

    if (!CreateProcess( NULL, Args, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE, 
        NULL, 
        NULL,
        &StartupInfo,
        &ProcessInfo))
    {
        return GetLastError();
    }
	if (WaitForSingleObject(ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
		return 0;
	}

    return -1;
}

/**
 * �v���O�C���I�[�v������B
 * @param[in]  szCommandLine �v���O�C�����ɓn���R�}���h���C���B
 * @param[in]  afxwInfo      ����w�̏��B
 * @param[out] openInfo      �I�[�v�����B
 * @retval     0�ȊO         �v���O�C���Ŏ��R�Ɏg����n���h���B���̑�API��handle�Ƃ��Ďg�p����B
 * @retval     NULL(0)       �I�[�v�����s�B
 */
HAFX WINAPI ApiOpen(LPCWSTR szCommandLine, const lpAfxwInfo afxwInfo, lpApiOpenInfo openInfo)
{
	lpPluginData pdata = (lpPluginData)GlobalAlloc(GMEM_FIXED, sizeof(PluginData));
	if (pdata == NULL) {
		return 0;
	}

	wchar_t dir[MAX_PATH];
	wchar_t path[MAX_PATH];
	memset(dir, 0, sizeof(dir));
	memset(path, 0, sizeof(path));
	GetTempPath(MAX_PATH, dir);
	GetTempFileName(dir, L"afx", 0, path);

	memset(pdata, 0, sizeof(PluginData));
	memcpy(&pdata->afx, afxwInfo, sizeof(AfxwInfo));

	GetModuleFileName(_instance, _ini_path, sizeof(_ini_path));
	int len = wcslen(_ini_path);
	_ini_path[len-1] = L'i';
	_ini_path[len-2] = L'n';
	_ini_path[len-3] = L'i';
	::GetPrivateProfileString(L"Config", L"title", L"SVN", openInfo->szTitle, sizeof(openInfo->szTitle), _ini_path);
	_item_max = ::GetPrivateProfileInt(L"Config", L"item_max", 64, _ini_path);
	if (_item_max < 16) {
		_item_max = 16;
	}

	wcscpy(openInfo->szBaseDir, L"");
	wcscpy(openInfo->szInitRelDir, L"");

	int ret;
	if (wcsncmp(szCommandLine, L"status", 6) == 0) {
	// STATUS
		pdata->mode = Mode_Status;
		ret = run(L"cmd /c svn status", path);
		wcsncpy(pdata->wszBaseDir, &szCommandLine[7], API_MAX_PATH_LENGTH);
	} else {
		GlobalFree(pdata);
		return NULL;
	}

	if (ret != 0) {
		GlobalFree(pdata);
		pdata = NULL;
	} else {
		// load
		int num = 0;
		FILE* fp = _wfopen(path, L"r, ccs=utf-8");
		if (fp == NULL) {
			GlobalFree(pdata);
			pdata = NULL;
		} else {
			wchar_t temp[MAX_LINE_SIZE];
			while (fgetws(temp, MAX_LINE_SIZE, fp) != NULL) {
				num++;
			}
			if (num != 0) {
				// STATUS
				if (pdata->mode == Mode_Status) {
					pdata->status_items = new StatusItem[num];
					pdata->status_count = num;
				}

				int idx = 0;
				fseek(fp, 0, SEEK_SET);
				while (fgetws(temp, MAX_LINE_SIZE, fp) != NULL) {
					// STATUS
					if (pdata->mode == Mode_Status) {
						if (wcslen(temp) > 1) {
							//int tm;
							wcscpy(pdata->status_items[idx].path, temp);
							TrimCarriageReturn(pdata->status_items[idx].path);
							idx++;	
						}
					}
				}
			}
		}
	}

	DeleteFile(path);

	return (HAFX)pdata;
}

/**
 * �v���O�C���N���[�Y����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiClose(HAFX handle)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}
	if (pdata != NULL) {
		GlobalFree(pdata);
	}
	return 1;
}

/**
 * szDirPath�Ŏw�肳�ꂽ���z�f�B���N�g���p�X�̐擪�A�C�e�����擾����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szDirPath     �擾�������f�B���N�g���t���p�X�B
 * @param[out] lpItemInfo    �擪�A�C�e���̏��B
 * @retval     1             �A�C�e�����������ꍇ
 * @retval     0             �G���[
 * @retval     -1            �����I��
 */
int  WINAPI ApiFindFirst(HAFX handle, LPCWSTR szDirPath, /*LPCWSTR szWildName,*/ lpApiItemInfo lpItemInfo)
{
	return ApiFindNext(handle, lpItemInfo);
}

/**
 * ApiFindFirst�Ŏ擾�����A�C�e���ȍ~�̃A�C�e�����擾����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[out] lpItemInfo    �擪�A�C�e���̏��B
 * @retval     1             �A�C�e�����������ꍇ
 * @retval     0             �G���[
 * @retval     -1            �����I��
 */
int  WINAPI ApiFindNext(HAFX handle, lpApiItemInfo lpItemInfo)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}
	if (lpItemInfo == NULL) {
		return -1;
	}

	memset(lpItemInfo->szItemName, 0, API_MAX_PATH_LENGTH);

	// STATUS
	if (pdata->mode == Mode_Status) {
		if (pdata->status_index >= pdata->status_count) {
			return -1;
		}

		wcscpy(lpItemInfo->szItemName, pdata->status_items[pdata->status_index].path);

		wchar_t path[API_MAX_PATH_LENGTH];
		wsprintf(path, L"%s\\%s", pdata->wszBaseDir, &pdata->status_items[pdata->status_index].path[8]);
		//ReplaceCharactor(path, L'/', L'\\');

		WIN32_FILE_ATTRIBUTE_DATA attrData;
		BOOL bRet = GetFileAttributesEx(path, GetFileExInfoStandard, &attrData);
		if (bRet) {
			if ( (attrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				lpItemInfo->dwAttr = FILE_ATTRIBUTE_SYSTEM;
				wcscat(lpItemInfo->szItemName, L"/");
			} else {
				lpItemInfo->dwAttr = attrData.dwFileAttributes;
			}
			lpItemInfo->ullItemSize = (((ULONGLONG)attrData.nFileSizeHigh) << 32) | (ULONGLONG)attrData.nFileSizeLow;
			lpItemInfo->ullTimestamp = attrData.ftLastWriteTime;
		} else {
			lpItemInfo->ullItemSize = pdata->status_index;
			lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
		}
		pdata->status_index++;
	} else {
		return -1;
	}

	return 1;
}

/**
 * �A�C�e�����g���q���ʎ��s����B
 * ����w��ENTER��SHIFT-ENTER���������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �A�C�e���̃t���p�X�B
 * @retval     2             ����w���ɏ�����C����B�iApiCopy�Ńe���|�����ɃR�s�[���Ă�����s)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiExecute(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t dir[MAX_PATH];
	wchar_t path[MAX_PATH];
	memset(dir, 0, sizeof(dir));
	memset(path, 0, sizeof(path));
	GetTempPath(MAX_PATH, dir);
	GetTempFileName(dir, L"afx", 0, path);

	if (pdata->mode == Mode_Status) {
		wchar_t wcmd[MAX_PATH+32];
		swprintf(wcmd, L"&EXCD -P\"%s\\%s\"", pdata->wszBaseDir, &szItemPath[9]);

		char cmd[MAX_PATH+32];
		wcstombs(cmd, wcmd, sizeof(cmd));

		AfxExec(pAfxApp, cmd);
	}

	return 1;
}

/**
 * �A�C�e�����g���q���ʎ��s����B
 * ����w��ENTER��SHIFT-ENTER���������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �A�C�e���̃t���p�X�B
 * @retval     2             ����w���ɏ�����C����B�iApiCopy�Ńe���|�����ɃR�s�[���Ă�����s)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiExecute2(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Status) {
		wchar_t cmd[1024];
		swprintf(cmd, sizeof(cmd), L"TortoiseProc.exe /command:diff /path:\"%s\\%s\"",
				pdata->wszBaseDir, &szItemPath[9]);
		if (run(cmd, L"NUL") == 0) {
			return 1;
		}
	}

	return 0;
}

/**
 * �A�C�e�����R�s�[����B
 * ����w�ŃR�s�[����������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szFromItem    �R�s�[���̃A�C�e�����B
 * @param[in]  szToPath      �R�s�[��̃t�H���_�B
 * @param[in]  lpPrgRoutine  �R�[���o�b�N�֐�(CopyFileEx�Ɠ��l)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Status) {
		wchar_t from[MAX_PATH];
		swprintf(from, MAX_PATH-1, L"%s\\%s", pdata->wszBaseDir, &szFromItem[8]);
		wchar_t to[MAX_PATH];
		swprintf(to, MAX_PATH-1, L"%s%s", szToPath, &szFromItem[8]);
		
		// �T�u�f�B���N�g��
		if (CreateSubDirectory(to) == FALSE) {
			return 0;
		}

		// �f�B���N�g���̏ꍇ�͂����܂�
		DWORD dwAttributes = GetFileAttributes(from);
		if (dwAttributes == 0xFFFFFFFF || dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return 1;
		}

		// �R�s�[
		int len = wcslen(szFromItem);
		if (szFromItem[len-1] != L'/') {
			if (::CopyFile(from, to, FALSE) == 0) {
				return 0;
			}
		}
		return 1;
	}

	return 0;
}

/**
 * @brief ����w�������I�ɗ��p����R�s�[�����B
 * ��Ƀv���O�C�����̃t�@�C�����J���ꍇ�ɁAAFXWTMP�ȉ��ɃR�s�[���邽�߂Ɏg����B
 * �������A���ۂɃR�s�[���邩�ǂ����̓v���O�C������ŁA����w��szOutputPath�Ŏw�肳�ꂽ�t�@�C�����J���B
 *
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szFromItem    �v���O�C�����̃A�C�e����
 * @param[in]  szToPath      �R�s�[��̃t�H���_(c:\Program Files\AFXWTMP.0\hoge.txt�����w�肳���)
 * @param[out] szOutputPath  �v���O�C�������ۂɃR�s�[�����t�@�C���p�X
 *                           szToPath�ɃR�s�[����ꍇ��szToPath��szOutputPath�ɃR�s�[���Ă���w�ɕԂ��B
 *                           szToPath�ɃR�s�[���Ȃ��ꍇ�A����w�ɊJ���Ăق����t�@�C���̃p�X���w�肷��B
 * @param[in]  dwOutPathSize szOutputPath�̃o�b�t�@�T�C�Y
 * @param[in]  lpPrgRoutine  �R�[���o�b�N�֐�(CopyFileEx�Ɠ��l)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiIntCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPWSTR szOutputPath, DWORD dwOutPathSize, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Status) {
		swprintf(szOutputPath, dwOutPathSize - 1, L"%s\\%s", pdata->wszBaseDir, &szFromItem[8]);
		return 1;
	}

	return 0;
}

/**
 * �A�C�e�����폜����B
 * ����w�ō폜������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �폜����A�C�e���̃t���p�X�B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiDelete(HAFX handle, LPCWSTR szFromItem)
{
	/*
	char cmd[MAX_PATH+32];

    wchar_t label = szFromItem[0];
    const wchar_t* p = wcsrchr(szFromItem, L')');
    if (p != NULL) {
        label = *(--p);
    }

    if ( (label >= L'A' && label <= L'Z' ) ||
            (label >= L'a' && label <= L'z' ) ) {
        sprintf(cmd, "&EJECT %C", label);
        AfxExec(pAfxApp, cmd);
        return 1;
    }
	*/
	return 0;
}

//------------------------------ �ȉ��|�X�g���[���` --------------------------------

/**
 * ����w������v���O�C���ɃA�C�e�����R�s�[����B
 * ����w�ŃR�s�[����������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szFromItem    �R�s�[���̃A�C�e�����B
 * @param[in]  szToPath      �R�s�[��̃t�H���_�B
 * @param[in]  lpPrgRoutine  �R�[���o�b�N�֐�(CopyFileEx�Ɠ��l)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiCopyFrom(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	return 0;
}

/**
 * �A�C�e�����ړ�����B
 * ����w�ňړ�����������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szFromPath    �ړ����̃t���p�X�B
 * @param[in]  szToPath      �ړ���̃t���p�X�B
 * @param[in]  direction     ApiDirection_A2P�͂���w����v���O�C�����Ɉړ�����ꍇ�B
 *                           ApiDirection_P2A�̓v���O�C�������炠��w�Ɉړ�����ꍇ�B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiMove(HAFX handle, LPCWSTR szFromPath, LPCWSTR szToPath, ApiDirection direction)
{
	return 0;
}

/**
 * �A�C�e�������l�[������B
 * ����w�Ń��l�[��������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szFromPath    ���l�[���O�̃t���p�X�B
 * @param[in]  szToName      ���l�[����̃t�@�C�����B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiRename(HAFX handle, LPCWSTR szFromPath, LPCWSTR szToName)
{
	return 0;
}

/**
 * �f�B���N�g�����쐬����B
 * ����w�Ńf�B���N�g�����쐬������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �쐬����f�B���N�g���̃t���p�X�B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiCreateDirectory(HAFX handle, LPCWSTR szItemPath)
{
	return 0;
}

/**
 * �f�B���N�g�����폜����B
 * ����w�Ńf�B���N�g�����폜������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �폜����f�B���N�g���̃t���p�X�B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiRemoveDirectory(HAFX handle, LPCWSTR szItemPath)
{
	return 0;
}

/**
 * �R���e�L�X�g���j���[�\��
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  hWnd          ����w�̃E�B���h�E�n���h���B
 * @param[in]  x             ���j���[�\��X���W�B
 * @param[in]  y             ���j���[�\��Y���W�B
 * @param[in]  szItemPath    �A�C�e���̃t���p�X�B
 * @retval     2             ����w���ɏ�����C����B�iApiCopy�Ńe���|�����ɃR�s�[���Ă���R���e�L�X�g���j���[?)
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiShowContextMenu(HAFX handle, const HWND hWnd, DWORD x, DWORD y, LPCWSTR szItemPath)
{
	return 0;
}

//------------------------------ �����֐� --------------------------------

BOOL CreateSubDirectory(wchar_t *szPath)
{
    wchar_t szMakePath[MAX_PATH+1];

    if (wcslen(szPath) > MAX_PATH) {
        return FALSE;
    }

    for (unsigned int i = 0; i < wcslen(szPath); i++) {
        szMakePath[i] = szPath[i];
        if (szPath[i] == L'\\' || szPath[i] == L'/') {
            szMakePath[i+1] = L'\0';
            if ( GetFileAttributes(szMakePath) == 0xFFFFFFFF ) {
                if (!CreateDirectory(szMakePath, NULL) ) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

void TrimCarriageReturn(wchar_t* wszBaseDir)
{
	size_t len = wcslen(wszBaseDir);
	if (wszBaseDir[len-1] == L'\n') {
		if (wszBaseDir[len-2] == L'\\' || wszBaseDir[len-2] == L'/') {
			wszBaseDir[len-2] = L'\0';
		} else {
			wszBaseDir[len-1] = L'\0';
		}
	} else {
		if (wszBaseDir[len-1] == L'\\' || wszBaseDir[len-1] == L'/') {
			wszBaseDir[len-1] = L'\0';
		}
	}
}

void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to)
{
	size_t len = wcslen(wszPath);
	for (size_t idx=0; idx<len; idx++) {
		if (wszPath[idx] == from) {
			wszPath[idx] = to;
		}
	}
}
