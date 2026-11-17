#include "dnd.h"

std::vector<std::filesystem::path> DropTarget::GetDroppedFiles()
{
    return m_droppedfiles;
}

std::vector<std::filesystem::path> DropTarget::GetDragFiles()
{
    return m_draggedFiles;
}

HRESULT __stdcall DropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = {};

    if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = static_cast<HDROP>(stg.hGlobal);
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

        std::vector<std::filesystem::path> filesBuffer;
        filesBuffer.reserve(fileCount); 

        for (UINT i = 0; i < fileCount; ++i)
        {
            UINT needed = DragQueryFile(hDrop, i, nullptr, 0);
            if (needed == 0) continue;

            std::vector<TCHAR> fileName(needed + 1, 0);

            UINT copied = DragQueryFile(hDrop, i, fileName.data(),
                static_cast<UINT>(fileName.size()));
            if (copied > 0 && copied < fileName.size())
                filesBuffer.emplace_back(fileName.data());
        }

        ReleaseStgMedium(&stg);

        m_draggedFiles = std::move(filesBuffer); 
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    dragStatus = DragStatus_enter;
    return S_OK;
}
HRESULT __stdcall DropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    dragStatus = DragStatus_hover;
    return S_OK;
}

HRESULT __stdcall DropTarget::DragLeave()
{
    dragStatus = DragStatus_leave;
    m_draggedFiles.clear();
    return S_OK;
}

HRESULT __stdcall DropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    std::filesystem::path droppedFilename; 

    FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = {};
    if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = static_cast<HDROP>(stg.hGlobal);
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

        for (UINT i = 0; i < fileCount; ++i)
        {
            UINT needed = DragQueryFile(hDrop, i, nullptr, 0);
            if (needed == 0) continue;

            std::vector<TCHAR> fileName(needed + 1, 0);

            if (DragQueryFile(hDrop, i, fileName.data(),
                static_cast<UINT>(fileName.size())) == 0)
                continue;

            EString name(fileName.data());
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return (char)std::tolower(c); });

            if (lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".dll") == 0)
            {
                droppedFilename = fileName.data();
                break;
            }
        }

        ReleaseStgMedium(&stg);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    if (!droppedFilename.empty())
    {
        const std::string s = droppedFilename.string();
        strncpy_s(InjectTarget::filePath, s.c_str(), _TRUNCATE);
    }

    dragStatus = DragStatus_drop;
    m_draggedFiles.clear();
    return S_OK;
}
