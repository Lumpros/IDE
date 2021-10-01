#include "Zoomer.h"
#include "SourceEdit.h"

#include <Richedit.h>

Zoomer::Zoomer(SourceEdit* pSourceEdit)
{
	m_pSourceEdit = pSourceEdit;
}

void Zoomer::AttachStatusBar(StatusBar* pStatusBar)
{
	m_pStatusBar = pStatusBar;
}

void Zoomer::SetStatusBarZoomText(void)
{
	wchar_t buf[8];
	wsprintf(buf, L" %d%%", m_iZoomNumerator);

	m_pStatusBar->SetText(buf, 2);
}

void Zoomer::RestrictNumeratorRange(void)
{
	if (m_iZoomNumerator > 500) 
	{
		m_iZoomNumerator = 500;
	}

	else if (m_iZoomNumerator < 10)
	{
		m_iZoomNumerator = 10;
	}
}

int Zoomer::RoundValueToTen(int value, short bWheelDelta) const
{
	const int iLastDigit = value % 10;

	value -= iLastDigit;

	if (bWheelDelta > 0)
	{
		value += 10;
	}

	else if (bWheelDelta < 0 && value >= 450)
	{
		value -= 10;
	}

	return value;
}

void Zoomer::Update(short bWheelDelta)
{
	DWORD dwNominator = NULL;
	DWORD dwDenominator = NULL;
	HWND hEditWindow = m_pSourceEdit->GetHandle();

	SendMessage(hEditWindow,
		        EM_GETZOOM,
		        reinterpret_cast<WPARAM>(&dwNominator),
		        reinterpret_cast<LPARAM>(&dwDenominator));

	if (dwDenominator != 0) {
		m_iZoomNumerator = (dwNominator * m_iZoomDenominator) / dwDenominator;
	}

	m_iZoomNumerator = RoundValueToTen(m_iZoomNumerator, bWheelDelta);
	RestrictNumeratorRange();

	SendMessage(hEditWindow,
		        EM_SETZOOM,
		        static_cast<WPARAM>(m_iZoomNumerator),
                static_cast<LPARAM>(m_iZoomDenominator));

	SetStatusBarZoomText();
}

void Zoomer::ZoomIn(void)
{
	m_iZoomNumerator += 10;

	if (m_iZoomNumerator > 500)
	{
		m_iZoomNumerator = 500;
	}

	else 
	{
		RequestZoom();
	}
}

void Zoomer::ZoomOut(void)
{
	m_iZoomNumerator -= 10;

	if (m_iZoomNumerator < 10)
	{
		m_iZoomNumerator = 10;
	}

	else
	{
		RequestZoom();
	}
}

void Zoomer::RequestZoom(void)
{
	LRESULT bAccepted =	SendMessage(m_pSourceEdit->GetHandle(),
		                            EM_SETZOOM,
		                            static_cast<WPARAM>(m_iZoomNumerator),
		                            static_cast<LPARAM>(m_iZoomDenominator));

	if (bAccepted) 
	{
		SetStatusBarZoomText();
	}
}

int Zoomer::GetZoomLevel(void) const
{
	return m_iZoomNumerator;
}