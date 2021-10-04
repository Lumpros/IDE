#include "FindReplace.h"
#include "Utility.h"

#include <Richedit.h>
#include <commdlg.h>

/// <summary>
/// Searches for the specified string either after or before the current selection.
/// If it finds it, then it is selected and it is scrolled into the window view of the edit control
/// If it isn't found, do one of two things:
/// 1. If wrapAround is true, then go back to top if we're searching downwards, or up if we're searching upwards
/// 2. If wrapAround is fase, then simply display a message that says there aren't more instances of the string
/// </summary>
/// <param name="lpszTarget"> The string that should be searched for </param>
/// <param name="hEditControl"> Handle to the edit control that contains the text to search </param>
/// <param name="dir"> The direction to search for the text relative to the current selection </param>
/// <param name="matchCase"> Whether the search is case sensitive or not </param>
/// <param name="wrapAround"> Whether the search should begin from the top when it ends </param>
/// <returns> Returns true if the string was found, returns false otherwise </returns>
bool FR::Find(const wchar_t* lpszTarget,
	          HWND hEditControl,
	          DWORD dwFlags)
{
	CHARRANGE cr;
	SendMessage(hEditControl, EM_EXGETSEL, NULL, (LPARAM)&cr);

	if (cr.cpMin == cr.cpMax) 
		cr.cpMin = cr.cpMax = 0;
		
	FINDTEXT ft;
	ft.lpstrText = lpszTarget;

	if (!(dwFlags & FR_DOWN)) 
	{
		ft.chrg.cpMin = cr.cpMin;
		ft.chrg.cpMax = 0;
	}

	else 
	{
		ft.chrg.cpMin = cr.cpMax;
		ft.chrg.cpMax = GetWindowTextLength(hEditControl);
	}
		
	int iPosition = SendMessage(hEditControl, EM_FINDTEXT, dwFlags, (LPARAM)&ft);

	if (iPosition != -1)
	{
		SendMessage(hEditControl, EM_SETSEL, iPosition, iPosition + lstrlen(lpszTarget));

		return true;
	}

	return false;
}

void FR::Replace(const wchar_t* lpszNewText,
	             HWND hEditControl,
	             DWORD dwFlags)
{
	DWORD dwStart = 0, dwEnd = 0;

	SendMessage(hEditControl,
		        EM_GETSEL,
		        reinterpret_cast<WPARAM>(&dwStart),
		        reinterpret_cast<LPARAM>(&dwEnd));

	if (dwStart != dwEnd)
	{
		SendMessage(hEditControl,
			        EM_REPLACESEL,
			        TRUE,
			        reinterpret_cast<LPARAM>(lpszNewText));

		Utility::UpdateUndoMenuButton(hEditControl);
	}
}

void FR::ReplaceAll(const wchar_t* lpszFind,
	                const wchar_t* lpszReplace,
	                HWND hEditControl,
	                DWORD dwFlags)
{
	SendMessage(hEditControl, EM_SETSEL, 0, 0);

	while (FR::Find(lpszFind, hEditControl, dwFlags))
		FR::Replace(lpszReplace, hEditControl, dwFlags);

	SendMessage(hEditControl, EM_SETSEL, 0, 0);
}