
#ifndef __view_drag_source_h__
#define __view_drag_source_h__

#pragma once

#include <objidl.h>

#include "base/ref_counted.h"

namespace view
{

    // IDropSource的基本实现. 处理当前拖拽过程中用户鼠标在目标上悬停的通知消息.
    // 对象告诉Windows拖拽是否还继续进行, 以提供正确的光标.
    class DragSource : public IDropSource,
        public base::RefCountedThreadSafe<DragSource>
    {
    public:
        DragSource();
        virtual ~DragSource() {}

        // 在下一次执行时停止拖拽操作. 不会同步停止拖拽(因为过程被Windows控制),
        // 但是会告诉Windows在下次移动的时候取消拖拽.
        void CancelDrag()
        {
            cancel_drag_ = true;
        }

        // IDropSource实现:
        HRESULT __stdcall QueryContinueDrag(BOOL escape_pressed, DWORD key_state);
        HRESULT __stdcall GiveFeedback(DWORD effect);

        // IUnknown实现:
        HRESULT __stdcall QueryInterface(const IID& iid, void** object);
        ULONG __stdcall AddRef();
        ULONG __stdcall Release();

    protected:
        virtual void OnDragSourceCancel() {}
        virtual void OnDragSourceDrop() {}
        virtual void OnDragSourceMove() {}

    private:
        // 如果想要取消拖拽操作, 设置为true.
        bool cancel_drag_;

        DISALLOW_COPY_AND_ASSIGN(DragSource);
    };

} //namespace view

#endif //__view_drag_source_h__