#include "FindReplace.h"
#include "Utility.h"

#include <Richedit.h>
#include <commdlg.h>

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

bool FR::Replace(const wchar_t* lpszNewText,
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
		
		return true;
	}

	return false;
}

bool FR::ReplaceAll(const wchar_t* lpszFind,
	                const wchar_t* lpszReplace,
	                HWND hEditControl,
	                DWORD dwFlags)
{
	bool was_anything_replaced = false;

	SendMessage(hEditControl, EM_SETSEL, 0, 0);

	while (FR::Find(lpszFind, hEditControl, dwFlags))
	{
		FR::Replace(lpszReplace, hEditControl, dwFlags);
		was_anything_replaced = true;
	}

	SendMessage(hEditControl, EM_SETSEL, 0, 0);

	return was_anything_replaced;
}