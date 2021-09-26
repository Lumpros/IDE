#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>

class Explorer;

class FileClipboard
{
public:
	void Copy(const wchar_t* lpszPath);
	void Cut(Explorer* pExplorer);
	void Paste(Explorer* pExplorer);

	bool CanPasteToDirectory(void);

private:
	void PasteFilesToTreeAndStorage(Explorer* pExplorer, HDROP hDrop);

private:
	bool m_ShouldDeleteOriginalAfterPaste = false;
};

