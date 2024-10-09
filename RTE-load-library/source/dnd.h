#pragma once
#include <oleidl.h>
#include <vector>
#include <filesystem>

enum DragStatus : int
{
    DragStatus_none = 0x00,
    DragStatus_enter = 0x01,
    DragStatus_hover = 0x02,
    DragStatus_drop = 0x03,
    DragStatus_leave = 0x04,
};

class DropTarget : public IDropTarget 
{
private:
    std::vector<std::filesystem::path> m_files;

public:
    std::vector<std::filesystem::path> GetFiles();

    DragStatus dragStatus;

    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    HRESULT STDMETHODCALLTYPE DragLeave();

    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    ULONG STDMETHODCALLTYPE AddRef() { return 1; }
    ULONG STDMETHODCALLTYPE Release() { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) 
    {
        if (riid == IID_IDropTarget) {
            *ppv = static_cast<IDropTarget*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

