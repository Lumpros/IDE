#include "ODButton.h"

#include <Uxtheme.h>
#include <windowsx.h>
#include <stdlib.h>

#pragma comment(lib, "UxTheme.lib")

#define TRANSITION_DURATION 100

static BP_ANIMATIONPARAMS CreateAnimationParamsStruct(void)
{
	BP_ANIMATIONPARAMS animParams;
	ZeroMemory(&animParams, sizeof(animParams));
	animParams.cbSize = sizeof(animParams);
	animParams.style = BPAS_LINEAR;
	animParams.dwDuration = TRANSITION_DURATION;

	return animParams;
}

static RECT CreateRectangleForBufferedAnimation(HWND hButtonWnd)
{
	RECT rcControl;
	GetWindowRect(hButtonWnd, &rcControl);
	OffsetRect(&rcControl, -rcControl.left, -rcControl.top);

	return rcControl;
}

/* Have to make adjustments before calling PtInRect */
static BOOLEAN PointInButtonRect(RECT rect, POINT point)
{
	rect.right += rect.left;
	rect.bottom += rect.top;

	point.x += rect.left;
	point.y += rect.top;

	return PtInRect(&rect, point);
}

static void OnMouseMove(ODButton* ptrButton, HWND hWnd)
{
	ptrButton->StartAnimation(TRUE, ptrButton->IsClicked());

	if (!ptrButton->isTrackingMouse)
	{
		ptrButton->isTrackingMouse = true;

		TRACKMOUSEEVENT tme = {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		tme.dwHoverTime = 1;
		_TrackMouseEvent(&tme);
	}
}

LRESULT CALLBACK CustomButtonSubclass(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	ODButton* ptrButton = reinterpret_cast<ODButton*>(dwRefData);

	if (ptrButton != NULL)
	{
		switch (message)
		{
		case WM_ERASEBKGND:
			return FALSE;

		case WM_MOUSEMOVE:
			OnMouseMove(ptrButton, hWnd);
			break;

		case WM_MOUSELEAVE:
			ptrButton->StartAnimation(FALSE, ptrButton->IsClicked());
			ptrButton->isTrackingMouse = false;
			break;

		case WM_LBUTTONDOWN:
			ptrButton->StartAnimation(TRUE, TRUE);
			break;

		case WM_LBUTTONUP:
			ptrButton->StartAnimation(TRUE, FALSE);
			break;
		}
	}

	return DefSubclassProc(hWnd, message, wParam, lParam);
}

ODButton::ODButton(HWND hWndParent, BUTTONINFO buttonInfo, WORD wID, DWORD dwExtraStyles)
{
	this->lpfnDrawFunc = buttonInfo.lpfnDrawFunc;
	this->lpszText = NULL;
	this->rect = buttonInfo.rect;
	this->wID = wID;

	if (buttonInfo.lpszText != NULL)
		this->lpszText = _wcsdup(buttonInfo.lpszText);

	hWnd = CreateWindowEx(
		NULL,
		L"Button",
		lpszText,
		WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | dwExtraStyles,
		rect.left,
		rect.top,
		rect.right,
		rect.bottom,
		hWndParent,
		(HMENU)wID,
		(HINSTANCE)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE),
		NULL
	);

	SetWindowSubclass(hWnd, CustomButtonSubclass, 0, (DWORD_PTR)this);
}

void ODButton::Draw(LPDRAWITEMSTRUCT pDIS)
{
	if (pDIS->hwndItem == hWnd)
	{
		BP_ANIMATIONPARAMS animParams = CreateAnimationParamsStruct();
		RECT rcControl = CreateRectangleForBufferedAnimation(hWnd);
		HDC hdcFrom, hdcTo;

		HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(
			hWnd, pDIS->hDC, &rcControl, BPBF_COMPATIBLEBITMAP,
			NULL, &animParams, &hdcFrom, &hdcTo
		);

		if (hbpAnimation)
		{
			DrawButtonStates(pDIS, hdcFrom, hdcTo);
			EndBufferedAnimation(hbpAnimation, TRUE);
		}

		else
		{
			lpfnDrawFunc(pDIS, lpszText, hasStartedAnimation, isClicked);
		}
	}
}

void ODButton::DrawButtonStates(LPDRAWITEMSTRUCT pDIS, HDC hdcFrom, HDC hdcTo)
{
	HFONT hFont = reinterpret_cast<HFONT>(GetCurrentObject(pDIS->hDC, OBJ_FONT));

	if (hdcFrom)
	{
		SelectObject(hdcFrom, hFont);
		pDIS->hDC = hdcFrom;
		lpfnDrawFunc(pDIS, lpszText, hasStartedAnimation, isClicked);
	}

	if (hdcTo)
	{
		SelectObject(hdcTo, hFont);
		pDIS->hDC = hdcTo;
		lpfnDrawFunc(pDIS, lpszText, hasStartedAnimation, isClicked);
	}
}

ODButton::~ODButton(void)
{
	if (lpszText != NULL)
	{
		free(lpszText);
	}
}

HWND ODButton::GetHandle(void) const
{
	return this ? hWnd : nullptr;
}

RECT ODButton::GetRect(void) const
{
	return rect;
}

void ODButton::StartAnimation(BOOLEAN hasStarted, BOOLEAN isClicked)
{
	if (hasStartedAnimation != hasStarted)
	{
		hasStartedAnimation = hasStarted;
		InvalidateRect(hWnd, NULL, FALSE);
	}

	else if (this->isClicked != isClicked)
	{
		this->isClicked = isClicked;
		InvalidateRect(hWnd, NULL, FALSE);
	}
}

BOOLEAN ODButton::IsClicked(void) const
{
	return isClicked;
}

void ODButton::SetRect(const RECT& rect)
{
	this->rect = rect;
	SetWindowPos(hWnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
}