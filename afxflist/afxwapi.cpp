/**
 * 1�s��1�̃t�@�C���p�X�������ꂽ�t�@�C����
 * ����w�̃t�@�C�����ɕ\������v���O�C��
 * 
 */
#include <string.h>
#include <stdio.h>
#include "afxwapi.h"
#include "version.h"
#include <wchar.h>

#define PLUGIN_NAME    L"afxflist"
#define PLUGIN_AUTHOR  L"yuratomo"

void TrimCarriageReturn(wchar_t* wszBaseDir);
void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to);

typedef struct _PluginData {
	AfxwInfo afx;
	wchar_t wszBaseDir[API_MAX_PATH_LENGTH];
	FILE* fp;
} PluginData, *lpPluginData;

//void _ShowLastError();

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

	pdata->fp = _wfopen(szCommandLine, L"r, ccs=utf-8");
	if (pdata->fp == NULL) {
		GlobalFree(pdata);
		return 0;
	}

	wcsncpy(openInfo->szBaseDir, L"", API_MAX_PATH_LENGTH);

	// �擪��#�t���Ńp�X�������Ă���΁A��f�B���N�g���ɂ���
	wchar_t temp[API_MAX_PATH_LENGTH];
	if (fgetws(temp, API_MAX_PATH_LENGTH, pdata->fp) != NULL) {
		if (temp[0] == L'#' && wcslen(temp) > 1) {
			wcsncpy(openInfo->szBaseDir, &temp[1], API_MAX_PATH_LENGTH);
		} else {
			fseek(pdata->fp, 0, SEEK_SET);
		}
	}

	wcsncpy(pdata->wszBaseDir, openInfo->szBaseDir, API_MAX_PATH_LENGTH);
	TrimCarriageReturn(pdata->wszBaseDir);
	ReplaceCharactor(pdata->wszBaseDir, L'\\', L'/');
	if (wcscmp(pdata->wszBaseDir, L"") != 0) {
		wcsncpy(openInfo->szBaseDir, pdata->wszBaseDir, API_MAX_PATH_LENGTH);
	}

	// �������΃f�B���N�g���̓��[�g("/")
	wcscpy(openInfo->szInitRelDir, L"/");
	wcscpy(openInfo->szTitle, L"��������");

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

	fclose(pdata->fp);
	pdata->fp = NULL;

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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	//�e�L�X�g���[�h����ftell�����܂��E���Ȃ��̂ŁA�擪�ɂ��ǂ��Ă���P�s�ǂݔ�΂��悤�ɂ���
	fseek(pdata->fp, 0, SEEK_SET);
	if (fgetws(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, pdata->fp) == NULL) {
		return -1;
	}

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

	wchar_t path[API_MAX_PATH_LENGTH];
	while (1) {
		if (fgetws(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, pdata->fp) == NULL) {
			if (feof(pdata->fp)) {
				return 0;
			} else {
				return -1;
			}
		}
		size_t len = wcslen(lpItemInfo->szItemName);
		if (len > 0 && lpItemInfo->szItemName[len-1] == L'\n') {
			lpItemInfo->szItemName[len-1] = L'\0';
		}

		wsprintf(path, L"%s/%s", pdata->wszBaseDir, lpItemInfo->szItemName);
		WIN32_FILE_ATTRIBUTE_DATA attrData;
		BOOL bRet = GetFileAttributesEx(path, GetFileExInfoStandard, &attrData);
		if (bRet == FALSE) {
			continue;
		}
		lpItemInfo->dwAttr = attrData.dwFileAttributes;
		lpItemInfo->ullItemSize = (((ULONGLONG)attrData.nFileSizeHigh) << 32) | (ULONGLONG)attrData.nFileSizeLow;
		lpItemInfo->ullTimestamp = attrData.ftLastWriteTime;
		break;
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

#if 0
	if (pdata->afx.MesPrint != NULL) {
		wchar_t szTest[512];
		_swprintf(szTest, L"execute %s.",szItemPath);
		pdata->afx.MesPrint(szTest);
	}
#endif

	/*
	if (ShellExecute(m_hWnd, L"open", szItemPath, NULL, NULL, SW_SHOWNORMAL) <= 32) {
		return 0;
	}
	*/
	int ret = 0;
#if 0
	if (pdata->afx.Exec != NULL) {
		wchar_t szCmd[512];
		_swprintf(szCmd, L"&EXCD -P\"%s/%s\"", pdata->wszBaseDir, szItemPath);
		BOOL bRet = pdata->afx.Exec(szCmd);
		if (bRet) {
			ret = 1;
		} else {
			ret = 0;
		}
	}
#endif

	return ret;
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
int  WINAPI ApiCopy(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	return ApiCopyTo(handle, szFromItem, szToPath, lpPrgRoutine);
}
int  WINAPI ApiCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t pathFrom[API_MAX_PATH_LENGTH];
	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wsprintf(pathFrom, L"%s/%s", pdata->wszBaseDir, szFromItem);
	const wchar_t* pFromName = wcsrchr(szFromItem, L'\\');
	if (pFromName == NULL) {
		pFromName = szFromItem;
	} else {
		pFromName++;
	}
	wsprintf(pathTo,   L"%s/%s", szToPath, pFromName);

	BOOL bCancel = FALSE;
	int ret = 0;
	BOOL bRet = ::CopyFileEx(pathFrom, pathTo, lpPrgRoutine, NULL, &bCancel, COPY_FILE_RESTARTABLE);
	if (bRet) {
		ret = 1;
	}

	return ret;
}

/**
 * ����w�������I�ɗ��p����R�s�[�����B
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

	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wsprintf(pathTo,   L"%s\\%s", pdata->wszBaseDir, szFromItem);

	// �o�b�t�@�T�C�Y�`�F�b�N
	size_t len = wcslen(pathTo);
	if (len > dwOutPathSize) {
		return 0;
	}

	// afxflist�̓t�@�C���R�s�[���Ȃ��ŁA���[�J���p�X��Ԃ��B
	wcsncpy(szOutputPath, pathTo, dwOutPathSize - 1);
	return 1;
}

/**
 * �A�C�e�����폜����B
 * ����w�ō폜������Ƃ��ɌĂяo�����B
 * @param[in]  handle        ApiOpen�ŊJ�����n���h���B
 * @param[in]  szItemPath    �폜����A�C�e���̃t���p�X�B
 * @retval     1             ����
 * @retval     0             �G���[
 */
int  WINAPI ApiDelete(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t path[API_MAX_PATH_LENGTH];
	wsprintf(path, L"%s\\%s", pdata->wszBaseDir, szItemPath);

	int ret = 0;
	BOOL bRet = ::DeleteFile(path);
	if (bRet) {
		ret = 1;
	}

	return ret;
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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	// ����w������v���O�C�����ւ̈ړ��͕s��
	if (direction == ApiDirection_A2P) {
		return 0;
	}

	wchar_t path[API_MAX_PATH_LENGTH];
	wsprintf(path, L"%s/%s", pdata->wszBaseDir, szFromPath);

	int ret = 0;
	BOOL bRet = ::MoveFile(path, szToPath);
	if (bRet) {
		ret = 1;
	}

	return ret;
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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t pathFrom[API_MAX_PATH_LENGTH];
	wsprintf(pathFrom, L"%s/%s", pdata->wszBaseDir, szFromPath);

	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wcscpy(pathTo, pathFrom);
	wchar_t* en = wcsrchr(pathTo, L'/');
	if (en == NULL) {
		return 0;
	}
	*(en+1) = L'\0';
	wcscat(pathTo, szToName);

	int ret = !_wrename(pathFrom, pathTo);

	return ret;
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
	return 2;
}

/*
void _ShowLastError()
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	  NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
	  (LPTSTR) &lpMsgBuf, 0, NULL);
	MessageBox(NULL, (const wchar_t*)lpMsgBuf, NULL, MB_OK);
	LocalFree(lpMsgBuf);
}
*/

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
