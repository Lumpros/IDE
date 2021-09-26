#include "FileClipboard.h"
#include "Utility.h"
#include "Explorer.h"

#include <Windows.h>
#include <atlbase.h>
#include <ShlObj.h>
#include <shellapi.h>

// Thanks Raymond
static HRESULT GetUIObjectOfFile(HWND hwnd, LPCWSTR pszPath, REFIID riid, void** ppv)
{
    *ppv = nullptr;

    LPITEMIDLIST pidl;
    SFGAOF sfgao;
    HRESULT hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, &sfgao);

    if (SUCCEEDED(hr)) 
    {
        IShellFolder* psf;
        LPCITEMIDLIST pidlChild;

        hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlChild);

        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
            psf->Release();
        }

        CoTaskMemFree(pidl);
    }

    return hr;
}

void FileClipboard::Copy(const wchar_t* lpszPath)
{
    m_ShouldDeleteOriginalAfterPaste = false;

    CComPtr<IDataObject> spdto;

    if (SUCCEEDED(GetUIObjectOfFile(nullptr, lpszPath, IID_PPV_ARGS(&spdto))))
    {
        OleSetClipboard(spdto);
    }
}

void FileClipboard::Cut(Explorer* pExplorer)
{
    std::wstring absolute_path;

    pExplorer->GetItemPath(
        pExplorer->GetTreeHandle(),
        pExplorer->GetRightClickedItem(),
        absolute_path
    );

    Copy(absolute_path.c_str());

    m_ShouldDeleteOriginalAfterPaste = true;
}

void FileClipboard::Paste(Explorer* pExplorer)
{
    if (CanPasteToDirectory())
    {
        if (OpenClipboard(NULL))
        {
            HGLOBAL hGlobal = reinterpret_cast<HGLOBAL>(GetClipboardData(CF_HDROP));

            if (hGlobal != nullptr)
            {
                HDROP hDrop = reinterpret_cast<HDROP>(GlobalLock(hGlobal));

                if (hDrop != nullptr)
                {
                    PasteFilesToTreeAndStorage(pExplorer, hDrop);
                    GlobalUnlock(hGlobal);
                }
            }

            CloseClipboard();
        }
    }
}

void FileClipboard::PasteFilesToTreeAndStorage(Explorer* pExplorer, HDROP hDrop)
{
    HWND hTree = pExplorer->GetTreeHandle();
    HTREEITEM hItem = pExplorer->GetRightClickedItem();

    wchar_t lpszFileName[MAX_PATH + 1];
    ZeroMemory(lpszFileName, sizeof(lpszFileName));

    std::wstring paste_directory;
    pExplorer->GetItemPath(hTree, hItem, paste_directory);
    paste_directory.push_back(L'\0');

    UINT uFileCount = DragQueryFile(hDrop, 0xFFFFFFFF, 0, 0);

    for (unsigned int i = 0; i < uFileCount; ++i)
    {
        UINT uFileNameLength = DragQueryFile(hDrop, i, 0, 0);
        DragQueryFile(hDrop, i, lpszFileName, uFileNameLength + 1);

        std::wstring name = Utility::GetFileNameFromPath(lpszFileName);

        if (Utility::IsPathDirectory(lpszFileName))
        {
            SHFILEOPSTRUCT op = { 0 };
            op.wFunc = FO_COPY;
            op.pTo = paste_directory.c_str();
            op.pFrom = lpszFileName;
            op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;

            if (!SHFileOperation(&op))
            {
                HTREEITEM hNewParent = Utility::AddToTree(
                    hTree,
                    hItem,
                    const_cast<wchar_t*>(name.c_str()),
                    true
                );

                pExplorer->ExploreDirectory(lpszFileName, hNewParent);

                if (m_ShouldDeleteOriginalAfterPaste) {
                    Utility::DeleteDirectory(lpszFileName);
                }
            }
        }

        else {
            // Because there are two '\0's at the end of the string we have to do this dumb thing
            std::wstring file_paste = paste_directory.substr(0, paste_directory.length() - 1);
            file_paste = file_paste + L'\\' + name;

            if (CopyFile(lpszFileName, file_paste.c_str(), false)) {
                Utility::AddToTree(hTree, hItem, const_cast<wchar_t*>(name.c_str()), false);
                if (m_ShouldDeleteOriginalAfterPaste)
                    DeleteFile(lpszFileName);
            }
        }
    }

    if (m_ShouldDeleteOriginalAfterPaste) {
        HTREEITEM& hItemToCut = pExplorer->GetItemToCut();
        TreeView_DeleteItem(hTree, hItemToCut);
        hItemToCut = nullptr;
    }
}

bool FileClipboard::CanPasteToDirectory(void)
{
    return IsClipboardFormatAvailable(CF_HDROP);
}