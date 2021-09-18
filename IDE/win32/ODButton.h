#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

/* First arguement: LPDRAWITEMSTRUCT which is the LPARAM reveiced in the WM_DRAWITEM message */
/* Second arguement: The button text. May be empty */
/* Third arguement:  Indicates whether or not the mouse is hovering over the button,
   so the DRAWFUNC knows whether it should draw the button's "final" state (therefore the name isFinal) */
   /* Fourth arguement: Indicates whether the button is clicked */
typedef void (*DRAWFUNC)(LPDRAWITEMSTRUCT, LPWSTR, BOOLEAN, BOOLEAN);

typedef struct tagBUTTONINFO
{
	DRAWFUNC lpfnDrawFunc;
	LPCWSTR  lpszText;
	RECT rect;
} BUTTONINFO;

class ODButton
{
private:
	LPWSTR lpszText;
	DRAWFUNC lpfnDrawFunc;
	WORD wID;
	HWND hWnd;
	RECT rect;
	BOOLEAN hasStartedAnimation = FALSE;
	BOOLEAN isClicked = FALSE;

	void DrawButtonStates(LPDRAWITEMSTRUCT pDIS, HDC hdcFrom, HDC hdcTo);

public:
	bool isTrackingMouse = false;

	void Draw(LPDRAWITEMSTRUCT pDIS);

	RECT GetRect(void) const;
	HWND GetHandle(void) const;

	BOOLEAN IsClicked(void) const;

	void SetRect(const RECT& rect);
	void StartAnimation(BOOLEAN hasStarted, BOOLEAN isClicked);

	inline WORD GetID(void) { return wID; }

	ODButton(HWND hWndParent, BUTTONINFO buttonInfo, WORD wID, DWORD dwExtraStyles = NULL);
	~ODButton(void);
};
