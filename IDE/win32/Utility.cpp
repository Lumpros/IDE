#include "Utility.h"
#include "resource.h"

#include <stdlib.h>
#include <CommCtrl.h>

#define DOT_NOT_FOUND -1

HFONT hFont = nullptr;

void Utility::CenterWindowRelativeToParent(HWND hWnd)
{
	HWND hParentWindow = GetParent(hWnd);
	RECT rcParentClient;

	/* Parent is the desktop */
	if (hParentWindow == NULL)
	{
		SetRect(&rcParentClient, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	}

	else
	{
		GetClientRect(hParentWindow, &rcParentClient);
	}

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	SetWindowPos(
		hWnd,
		nullptr,
		(rcParentClient.right - rcClient.right) / 2, 
		(rcParentClient.bottom - rcClient.bottom) / 2, 
		NULL,
		NULL,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

bool Utility::IsNumber(const wchar_t* lpszText)
{
	const int value = _wtoi(lpszText);
	const size_t length = lstrlen(lpszText);

	if (value == 0)
	{
		for (size_t i = 0; i < length; ++i)
		{
			if (lpszText[i] != L'0')
			{
				return false;
			}
		}
	}

	return true;
}

float Utility::GetScaleForDPI(HWND hWnd)
{
	return GetDpiForWindow(hWnd) / 96.0f;
}

HFONT Utility::GetStandardFont(void)
{
	return hFont;
}

void Utility::UpdateFont(HWND hWnd)
{
	if (hFont != nullptr) {
		DeleteObject(hFont);
		hFont = nullptr;
	}

	hFont = CreateFont(
		GetStandardFontHeight(hWnd),
		0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Segoe UI"
	);
}

int Utility::GetStandardFontHeight(HWND hWnd)
{
	return static_cast<int>(16 * GetScaleForDPI(hWnd));
}

std::wstring Utility::GetFileExtension(const wchar_t* lpszFileName)
{
	const size_t length = lstrlen(lpszFileName);
	int dot_index = DOT_NOT_FOUND;

	std::wstring extension;

	for (size_t i = 0; i < length; ++i)
	{
		if (lpszFileName[i] == L'.')
		{
			dot_index = i;
			break;
		}
	}

	if (dot_index != DOT_NOT_FOUND)
	{
		extension.assign(lpszFileName + dot_index, lpszFileName + length);
	}

	return extension;
}

void Utility::DrawTextCentered(HDC hDC, const RECT& rc, const wchar_t* lpszText)
{
	const int length = lstrlen(lpszText);
	SIZE size;
	GetTextExtentPoint32(hDC, lpszText, length, &size);

	TextOut(
		hDC,
		(rc.right - size.cx) / 2,
		(rc.bottom - size.cy) / 2,
		lpszText,
		length
	);
}

std::wstring Utility::GetFileNameFromPath(const std::wstring& path)
{
	const size_t position = path.find_last_of(L'\\');

	if (position != std::wstring::npos)
	{
		return path.substr(position + 1, path.size() - position);
	}

	return path;
}

bool Utility::IsPathDirectory(const std::wstring& path)
{
	struct _stat64i32 s;
	_wstat64i32(path.c_str(), &s);

	return s.st_mode & S_IFDIR;
}

HTREEITEM Utility::AddToTree(HWND hTreeView, HTREEITEM hParent, LPWSTR lpszItem, bool isDirectory)
{
	TVITEM tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.pszText = lpszItem;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.iImage = isDirectory ? 0 : 1;
	tvItem.iSelectedImage = isDirectory ? 0 : 1;

	TVINSERTSTRUCT tvInsert = {};
	tvInsert.item = tvItem;
	tvInsert.hInsertAfter = TreeView_GetPrevSibling(hTreeView, TreeView_GetChild(hTreeView, hParent));
	tvInsert.hParent = hParent;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

HTREEITEM Utility::SetItemAsTreeRoot(HWND hTreeView, LPWSTR lpszItem)
{
	TVITEM tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.pszText = lpszItem;
	tvItem.iImage = 0;
	tvItem.iSelectedImage = 0;

	TVINSERTSTRUCT tvInsert = {};
	tvInsert.hParent = TVI_ROOT;
	tvInsert.hInsertAfter = TVI_FIRST;
	tvInsert.item = tvItem;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

void Utility::DeleteDirectory(const wchar_t* lpszDirectory)
{
	std::wstring wstr = lpszDirectory;
	wstr.append(L"\\*");

	WIN32_FIND_DATA find_data;
	HANDLE hFind = FindFirstFile(wstr.c_str(), &find_data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			std::wstring to_delete = lpszDirectory;
			to_delete += L'\\';
			to_delete += find_data.cFileName;

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				DeleteDirectory(to_delete.c_str());
			}

			else
			{
				DeleteFile(to_delete.c_str());
			}

		} while (FindNextFile(hFind, &find_data));

		FindClose(hFind);
	}

	RemoveDirectory(lpszDirectory);
}

void Utility::SetMenuItemsState(HMENU hMenu, UINT uState)
{
	EnableMenuItem(hMenu, ID_EDIT_FIND, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_EDIT_FINDNEXT, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_EDIT_FINDPREVIOUS, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_EDIT_GOTO, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_EDIT_REPLACE, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_EDIT_SELECTALL, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_ZOOM_ZOOMIN, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_ZOOM_ZOOMOUT, uState | MF_BYCOMMAND);
	EnableMenuItem(hMenu, ID_ZOOM_RESTOREDEFAULTZOOM, uState | MF_BYCOMMAND);

	if (uState == MF_GRAYED)
	{
		EnableMenuItem(hMenu, ID_EDIT_UNDO, uState | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_EDIT_CUT, uState | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_EDIT_COPY, uState | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_EDIT_DELETE, uState | MF_BYCOMMAND);
	}
}

void Utility::UpdateUndoMenuButton(HWND hEditWindow)
{
	HMENU hMenu = GetMenu(GetAncestor(hEditWindow, GA_ROOT));
	DWORD uEnable = SendMessage(hEditWindow, EM_CANUNDO, NULL, NULL) ? MF_ENABLED : MF_GRAYED;
	EnableMenuItem(hMenu, ID_EDIT_UNDO, MF_BYCOMMAND | uEnable);
}

void Utility::RefreshPasteMenuButton(HMENU hMenu)
{
	UINT uEnable = IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED;

	EnableMenuItem(hMenu, ID_EDIT_PASTE, uEnable | MF_BYCOMMAND);
}