#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <strsafe.h>
#include <atlbase.h>

/**********************************************************************
* Function......: CreateShortcut
* Parameters....: lpszFileName - string that specifies a valid file name
*				  lpszDesc - string that specifies a description for a shortcut
*				  lpszShortcutPath - string that specifies a path and file name of a shortcut
* Returns.......: S_OK on success, error code on failure
* Description...: Creates a Shell link object (shortcut)
**********************************************************************/
HRESULT CreateShortcut(/*in*/ LPCTSTR lpszFileName,
	/*in*/ LPCTSTR lpszDesc,
	/*in*/ LPCTSTR lpszShortcutPath);

/*********************************************************************
* Function......: ResolveShortcut
* Parameters....: lpszShortcutPath - string that specifies a path and file name of a shortcut
*				  lpszFilePath - string that will contain a file name
* Returns.......: S_OK on success, error code on failure
* Description...: Resolves a Shell link object (shortcut)
*********************************************************************/
HRESULT ResolveShortcut(/*in*/ LPCTSTR lpszShortcutPath,
	/*out*/ LPTSTR lpszFilePath);
