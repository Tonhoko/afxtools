/**
 * ����w�̃t�@�C�����Ƀt�H���_������\������v���O�C��
 * 
 */
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "afxwapi.h"
#include "afxcom.h"
#include "version.h"

#define PLUGIN_NAME    L"afxhistory"
#define PLUGIN_AUTHOR  L"yuratomo"

typedef struct _HistoryItem {
	wchar_t title[API_MAX_PATH_LENGTH];
	char    cmd[API_MAX_PATH_LENGTH];
} HistoryItem;
typedef struct _PluginData {
	AfxwInfo afx;
	HistoryItem* menu;
	int count;
	int index;
} PluginData, *lpPluginData;

void _format_title(wchar_t* title);

IDispatch* pAfxApp = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
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

	memset(pdata, 0, sizeof(PluginData));
	memcpy(&pdata->afx, afxwInfo, sizeof(AfxwInfo));

	wcscpy(openInfo->szBaseDir, L"");
	wcscpy(openInfo->szInitRelDir, L"");
	wcsncpy(openInfo->szTitle, szCommandLine, sizeof(openInfo->szTitle));

	char path2[MAX_PATH];
	VARIANT result;

	// ���ԍ�
	AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"SrcWin", 0);
	int win = result.intVal;

	// ����
	wchar_t wbuf[32];
	wsprintfW(wbuf, L"%d", win);
	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wbuf);
	AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDirCount", 1, x);
	int nHisDirCnt = result.intVal;
	pdata->menu = new HistoryItem[nHisDirCnt];

	// ����ǂݍ���
	int idx=0;
	for (idx=0; idx<nHisDirCnt; idx++) {
		VARIANT y;
		y.vt = VT_BSTR;
		wsprintfW(wbuf, L"%d", idx);
		y.bstrVal = ::SysAllocString(wbuf);
		AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDir", 2, y, x);
		if (result.bstrVal == NULL) continue;
		wcstombs(path2, result.bstrVal, sizeof(path2));
		sprintf(pdata->menu[idx].cmd,  "&EXCD -P\"%s\"", path2);
		wcscpy(pdata->menu[idx].title, result.bstrVal);
		// �^�C�g�����`
		//_format_title(pdata->menu[idx].title);
	}
	pdata->index = 0;
	pdata->count = idx;

	return (HAFX)pdata;
}

void _format_title(wchar_t* title)
{
	int en_cnt = 0;
	int len = (int)wcslen(title);
	for (int idx=len-1; idx>=0; idx--) {
		if (title[idx] == L'\\') {
			en_cnt++;
			if (en_cnt == 2) {
				title[idx+1] = L' ';
				break;
			}
		} else {
			title[idx+1] = title[idx];
		}
	}
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
	if (pdata->menu != NULL) {
		delete [] pdata->menu;
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

	if (pdata->index >= pdata->count) {
		return -1;
	}

	swprintf(lpItemInfo->szItemName, L"%02d %s", pdata->index, pdata->menu[pdata->index].title);
	lpItemInfo->ullItemSize = pdata->index;
	lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;

	pdata->index++;
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
	int idx = -1;
	wchar_t buf[1024];

	const wchar_t* start = wcschr(szItemPath, L'/');
	if (start != NULL) {
		start++;
	} else {
		start = szItemPath;
	}

	swscanf(start, L"%d %s", &idx, buf);
	
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (idx < 0 || idx >= pdata->count) {
		return 0;
	}
	AfxExec(pAfxApp, pdata->menu[idx].cmd);
	return 1;
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



