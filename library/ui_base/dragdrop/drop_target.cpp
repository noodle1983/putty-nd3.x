
#include "drop_target.h"

#include <shlobj.h>

#include "base/logging.h"

namespace ui
{

    IDropTargetHelper* DropTarget::cached_drop_target_helper_ = NULL;

    DropTarget::DropTarget(HWND hwnd) : hwnd_(hwnd),
        suspended_(false), ref_count_(0)
    {
        DCHECK(hwnd);
        HRESULT result = RegisterDragDrop(hwnd, this);
        DCHECK(SUCCEEDED(result));
    }

    DropTarget::~DropTarget() {}

    // static
    IDropTargetHelper* DropTarget::DropHelper()
    {
        if(!cached_drop_target_helper_)
        {
            CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER,
                IID_IDropTargetHelper,
                reinterpret_cast<void**>(&cached_drop_target_helper_));
        }
        return cached_drop_target_helper_;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // DropTarget, IDropTarget implementation:
    HRESULT DropTarget::DragEnter(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD* effect)
    {
        // 告诉辅助器我们进入了, 以便它能更新拖拽图像.
        IDropTargetHelper* drop_helper = DropHelper();
        if(drop_helper)
        {
            drop_helper->DragEnter(GetHWND(), data_object,
                reinterpret_cast<POINT*>(&cursor_position), *effect);
        }        

        // 不能在同一窗口拖拽.
        if(suspended_)
        {
            *effect = DROPEFFECT_NONE;
            return S_OK;
        }

        current_data_object_ = data_object;
        *effect = OnDragEnter(current_data_object_, key_state, cursor_position,
            *effect);
        return S_OK;
    }

    HRESULT DropTarget::DragOver(DWORD key_state,
        POINTL cursor_position,
        DWORD* effect)
    {
        // 告诉辅助器我们移动了, 以便它能更新拖拽图像.
        IDropTargetHelper* drop_helper = DropHelper();
        if(drop_helper)
        {
            drop_helper->DragOver(reinterpret_cast<POINT*>(&cursor_position),
                *effect);
        }

        if(suspended_)
        {
            *effect = DROPEFFECT_NONE;
            return S_OK;
        }

        *effect = OnDragOver(current_data_object_, key_state, cursor_position,
            *effect);
        return S_OK;
    }

    HRESULT DropTarget::DragLeave()
    {
        // 告诉辅助器我们移出了, 以便它能更新拖拽图像.
        IDropTargetHelper* drop_helper = DropHelper();
        if(drop_helper)
        {
            drop_helper->DragLeave();
        }

        if(suspended_)
        {
            return S_OK;
        }

        OnDragLeave(current_data_object_);

        current_data_object_ = NULL;
        return S_OK;
    }

    HRESULT DropTarget::Drop(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD* effect)
    {
        // 告诉辅助器我们拖放了, 以便它能更新拖拽图像.
        IDropTargetHelper* drop_helper = DropHelper();
        if(drop_helper)
        {
            drop_helper->Drop(current_data_object_,
                reinterpret_cast<POINT*>(&cursor_position), *effect);
        }

        if(suspended_)
        {
            *effect = DROPEFFECT_NONE;
            return S_OK;
        }

        *effect = OnDrop(current_data_object_, key_state, cursor_position,
            *effect);
        return S_OK;
    }

    HRESULT DropTarget::QueryInterface(const IID& iid, void** object)
    {
        *object = NULL;
        if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget))
        {
            *object = this;
        }
        else
        {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG DropTarget::AddRef()
    {
        return ++ref_count_;
    }

    ULONG DropTarget::Release()
    {
        if(--ref_count_ == 0)
        {
            delete this;
            return 0U;
        }
        return ref_count_;
    }

    DWORD DropTarget::OnDragEnter(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        return DROPEFFECT_NONE;
    }

    DWORD DropTarget::OnDragOver(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        return DROPEFFECT_NONE;
    }

    void DropTarget::OnDragLeave(IDataObject* data_object) {}

    DWORD DropTarget::OnDrop(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        return DROPEFFECT_NONE;
    }

} //namespace ui