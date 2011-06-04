
#ifndef __view_drop_target_win_h__
#define __view_drop_target_win_h__

#pragma once

#include "drop_helper.h"
#include "drop_target.h"

namespace view
{

    class RootView;
    class View;

    // DropTargetWin负责WidgetWin的拖拽管理. 它把Windows OLE的拖放消息转换
    // 成View的拖放消息.
    //
    // DropTargetWin使用DropHelper管理目标视图的拖放消息.
    class DropTargetWin : public DropTarget
    {
    public:
        explicit DropTargetWin(RootView* root_view);
        virtual ~DropTargetWin();

        // 如果正在执行拖拽, 且视图就是当前拖放目标视图, 拖放目标重置为null.
        // 当视图从RootView中移除时调用, 确保目标视图不为非法值.
        void ResetTargetViewIfEquals(View* view);

    protected:
        virtual DWORD OnDragOver(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

        virtual void OnDragLeave(IDataObject* data_object);

        virtual DWORD OnDrop(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

    private:
        view::DropHelper helper_;

        DISALLOW_COPY_AND_ASSIGN(DropTargetWin);
    };

} //namespace view

#endif //__view_drop_target_win_h__