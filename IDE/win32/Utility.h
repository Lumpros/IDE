#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <CommCtrl.h>
#include <string>

#define SAFE_DELETE_PTR(ptr) if (ptr) { delete ptr; ptr = nullptr; }
#define SAFE_DELETE_GDIOBJ(obj) if (obj) { DeleteObject(obj); obj = nullptr; }

template <typename T> 
inline T* GetAssociatedObject(HWND hWnd)
{
	return reinterpret_cast<T*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

namespace Utility
{
	// Pretty self explanatory
	extern void CenterWindowRelativeToParent(HWND hWnd);

	/// <summary>
	/// Takes in an absolute path, finds the last backslash,
	/// and returns the string after it (=> the file name)
	/// </summary>
	/// <param name="path"> Absolute path of file </param>
	/// <returns> File name </returns>
	extern std::wstring GetFileNameFromPath(const std::wstring& path);

	/// <summary>
	/// Decides whether the text passed is a number
	/// Do this because _wtoi returns 0 when it isn't a number
	/// </summary>
	extern bool IsNumber(const wchar_t* lpszText);

	/// <summary>
	/// Calculates how much the UI should be scaled by
	/// based on the window DPI
	/// </summary>
	/// <param name="hWnd"> Handle to any window belonging to the process </param>
	/// <returns> Scale </returns>
	extern float GetScaleForDPI(HWND hWnd);

	/// <summary>
	/// Returns the font that is saved globally that is a standard size
	/// for any text that we need displayed
	/// </summary>
	/// <returns> Font </returns>
	extern HFONT GetStandardFont(void);

	/// <summary>
	/// Calculates and returns the height of the standard font that is saved globally
	/// </summary>
	/// <param name="hWnd"> Handle to any window in the process </param>
	extern int GetStandardFontHeight(HWND hWnd);

	/// <summary>
	/// Sets the global (standard) font size based on the current
	/// DPI of the window
	/// </summary>
	/// <param name="hWnd"> Any window belonging to the process </param>
	extern void UpdateFont(HWND hWnd);

	/// <summary>
	/// Takes in an absolute path or just a file name with an extension
	/// and returns the file extension with the dot (e.g. 'C:\\app.exe' -> '.exe')
	/// </summary>
	/// <param name="lpszFileName"> Path/File </param>
	/// <returns> File Extension </returns>
	extern std::wstring GetFileExtension(const wchar_t* lpszFileName);

	// Pretty self explanatory
	extern void DrawTextCentered(HDC hDC, const RECT& rc, const wchar_t* lpszText);

	/// <summary>
	/// Decides which tree item is under the mouse cursor, and
	/// get its absolute path
	/// </summary>
	/// <param name="hTreeWindow"> Handle to Tree View window </param>
	/// <param name="ptClick"> Cursor point relative to the top left of the Tree Viee Window </param>
	/// <param name="path"> Receives the absolute path of the clicked tree item </param>
	/// <returns> Tree item under mouse cursor </returns>
	extern HTREEITEM GetClickedTreeItemPath(
		_In_  HWND hTreeWindow,
		_In_  POINT ptClick,
		_Out_ std::wstring& path
	);

	/// <summary>
	/// Get the absolute path of a treeitem
	/// </summary>
	/// <param name="hTreeWindow"> Handle to Tree View window </param>
	/// <param name="hItem"> Handle to a Tree View item </param>
	/// <param name="path"> Receives the absolute path of the clicked tree item </param>
	/// <returns> Returns the handle of the tree item (same as hItem) </returns>
	extern HTREEITEM GetItemPath(
		_In_ HWND hTreeWindow,
		_In_ HTREEITEM hItem,
		_Out_ std::wstring& path
	);
}