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
    FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg;
    if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = (HDROP)stg.hGlobal;
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        std::vector<std::filesystem::path> filesBuffer;
        filesBuffer.resize(fileCount);

        for (UINT i = 0; i < fileCount; i++) {
            TCHAR fileName[MAX_PATH];
            if (DragQueryFile(hDrop, i, fileName, MAX_PATH)) 
            {
                filesBuffer[i] = fileName;
            }
        }
        DragFinish(hDrop);
        ReleaseStgMedium(&stg); 
        m_draggedFiles = filesBuffer;
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
    STGMEDIUM stg;
    if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = (HDROP)stg.hGlobal;
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

        for (UINT i = 0; i < fileCount; i++) {
            TCHAR fileName[MAX_PATH];
            if (DragQueryFile(hDrop, i, fileName, MAX_PATH))
            {
                if (EString(fileName).ends_with(".dll"))
                {
                    droppedFilename = fileName;
                    break;
                }
            }
        }
        DragFinish(hDrop);
        ReleaseStgMedium(&stg);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    strcpy_s(InjectTarget::filePath, droppedFilename.string().data());

    dragStatus = DragStatus_drop;
    m_draggedFiles.clear();
    return S_OK;
}
