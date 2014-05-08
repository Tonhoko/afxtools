/**
 * ����w�̃t�@�C�����Ƀh���C�u�ꗗ��\������v���O�C��
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

#define PLUGIN_NAME    L"afxdrive"
#define PLUGIN_AUTHOR  L"yuratomo"

typedef struct _PluginData {
	AfxwInfo afx;
	DWORD dwDrives;
	int index;
	int sindex;
} PluginData, *lpPluginData;

// ����f�B���N�g����`
static const wchar_t* _SpecialDirName [] = {
	L"0) My Document",
	L"1) Desktop",
    L"2) System32",
    L"3) Program Files",
};
static int _SpecialDirTable [] = {
	CSIDL_PERSONAL,            // My Document
	CSIDL_DESKTOPDIRECTORY,    // Desktop
    -1,                        // System32
    -2,                        // Program Files
};
static int _SpecialDirTableCount = sizeof(_SpecialDirTable) / sizeof(int);


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

	pdata->dwDrives = GetLogicalDrives();
	pdata->index = 0;
	pdata->sindex = 0;

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

	for ( ; pdata->index<26 ; pdata->index++ ){
		if ( pdata->dwDrives & (1 << pdata->index) ){
			break;
		}
	}

	if (pdata->index >= 26) {
		if (pdata->sindex < _SpecialDirTableCount) {
			swprintf(lpItemInfo->szItemName, L"%s", _SpecialDirName[pdata->sindex]);
			lpItemInfo->ullItemSize = 0;
			lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
			pdata->sindex++;

			return 1;
		}
		return -1;
	}

	wchar_t drive[8];
	swprintf(drive, L"%c:\\", (pdata->index + TEXT('A')) );

	wchar_t type[64];
	memset(type, 0, sizeof(type));
	int drv_type = GetDriveType(drive);
	switch(drv_type) {
	case DRIVE_REMOVABLE:
		swprintf(type, L"REMOVABLE");
		break;
	case DRIVE_FIXED:
		swprintf(type, L"HDD");
		break;
	case DRIVE_REMOTE:
		swprintf(type, L"REMOTE");
		break;
	case DRIVE_CDROM:
		swprintf(type, L"CD-ROM");
		break;
	case DRIVE_RAMDISK:
		swprintf(type, L"RAM");
		break;
	}

	wchar_t VolumeName[1000];
	wchar_t SystemName[1000];
	wcscpy(VolumeName, L"");
	wcscpy(SystemName, L"");

	// FDD�΍�
	//   - REMOVABLE��A,B�h���C�u�͑ΏۊO
	if (drv_type != DRIVE_REMOVABLE || pdata->index > 1) {
		ULARGE_INTEGER Ava, Total, Free;
		if (GetDiskFreeSpaceEx(drive, &Ava, &Total, &Free)) {
			lpItemInfo->ullItemSize = Total.QuadPart;
		}

		DWORD SerialNumber;
		DWORD FileNameLength;
		DWORD Flags;
		BOOL ret = GetVolumeInformation(
			drive,
			VolumeName,
			sizeof(VolumeName),
			&SerialNumber,
			&FileNameLength,
			&Flags,
			SystemName,
			sizeof(SystemName));
		if (ret == FALSE) {
			memset(SystemName, 0, sizeof(SystemName));
			memset(VolumeName, 0, sizeof(VolumeName));
		}
	} else {
		lpItemInfo->ullItemSize = 0;
	}

	swprintf(lpItemInfo->szItemName, L"%C) %s/%s/%s", 
			(pdata->index + TEXT('A')), VolumeName, type, SystemName);
	//lpItemInfo->ullTimestamp = 0;
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
	char cmd[MAX_PATH+32];
    wchar_t label = szItemPath[0];
    const wchar_t* p = wcschr(szItemPath, L')');
    if (p != NULL) {
        label = *(--p);
    }

    if ( (label >= L'A' && label <= L'Z' ) ||
            (label >= L'a' && label <= L'z' ) ) {
        sprintf(cmd, "&EXCD -P\"%C:\\\"", label);
        AfxExec(pAfxApp, cmd);
        return 1;
    }

    int idx = label - L'0';
    if (idx >= 0 && idx <= _SpecialDirTableCount) {
        char path[MAX_PATH+1];
        wchar_t wpath[MAX_PATH+1];
        if (_SpecialDirTable[idx] == -1) {
            GetSystemDirectory(wpath, sizeof(wpath)/sizeof(wpath[0]));
        } else if (_SpecialDirTable[idx] == -2) {
            wcscpy(wpath, L"$V\"ProgramFiles\"");
        } else {
            LPITEMIDLIST PidList;
            SHGetSpecialFolderLocation(NULL, _SpecialDirTable[idx], &PidList);
            SHGetPathFromIDList(PidList, wpath);
            CoTaskMemFree(PidList);
        }

        wcstombs(path, wpath, sizeof(path));
        sprintf(cmd, "&EXCD -P\"%s\\\"", path);
        AfxExec(pAfxApp, cmd);
        return 1;
    }
	return 0;
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
	return ApiExecute(handle, szItemPath);
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
	wchar_t label = szFromItem[0];
	const wchar_t* p = wcschr(szFromItem, L')');
	if (p != NULL) {
		label = *(--p);
	}

	wchar_t path[MAX_PATH+1];
	swprintf(path, L"%s%s", szToPath, PLUGIN_NAME);

	if ( (label >= L'A' && label <= L'Z' ) ||
			(label >= L'a' && label <= L'z' ) ) {

		FILE* fp = _wfopen(path, L"w");
		if (fp == NULL) {
			return 1;
		}

		wchar_t drive[8];
		swprintf(drive, L"%c:\\", label );

		ULARGE_INTEGER Ava, Total, Free;
		GetDiskFreeSpaceEx(drive, &Ava, &Total, &Free);
		fwprintf(fp, L"<< %C drive >>\n", label);
		fwprintf(fp, L"Free Space:  %8d M\n", Free.QuadPart / 1024 / 1024 );
		fwprintf(fp, L"Disk Bytes:  %8d M\n", Total.QuadPart / 1024 / 1024 );
		fclose(fp);
		wcsncpy(szOutputPath, path, dwOutPathSize - 1);
		return 1;
	}

	int idx = label - L'0';
	if (idx >= 0 && idx <= _SpecialDirTableCount) {
        wchar_t wpath[MAX_PATH+1];
        if (_SpecialDirTable[idx] == -1) {
            GetSystemDirectory(wpath, sizeof(wpath)/sizeof(wpath[0]));
        } else if (_SpecialDirTable[idx] == -2) {
            wcscpy(wpath, L"$V\"ProgramFiles\"");
        } else {
            LPITEMIDLIST PidList;
            SHGetSpecialFolderLocation(NULL, _SpecialDirTable[idx], &PidList);
            SHGetPathFromIDList(PidList, wpath);
            CoTaskMemFree(PidList);
        }
		FILE* fp = _wfopen(path, L"w");
		if (fp == NULL) {
			return 1;
		}
		fwprintf(fp, L"Path:\n%s\n", wpath);
		fclose(fp);
		wcsncpy(szOutputPath, path, dwOutPathSize - 1);
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



