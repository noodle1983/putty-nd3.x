
#ifndef __view_drop_helper_h__
#define __view_drop_helper_h__

#pragma once

#include <objidl.h>

#include "base/basic_types.h"

namespace gfx
{
    class Point;
}

namespace view
{

    class View;

    // DropHelper为管理拖拽过程中目标视图提供支持. DropHelper在与系统拖放交互的类
    // 中使用. 系统类在鼠标移动时会调用OnDragOver, 在拖放结束时调用OnDragExit或者
    // OnDrop.
    class DropHelper
    {
    public:
        explicit DropHelper(View* root_view);
        ~DropHelper();

        // 当前拖放事件的目标视图, 可以为NULL.
        View* target_view() const { return target_view_; }

        // 返回创建DropHelper的RootView.
        View* root_view() const { return root_view_; }

        // 重置target_view_为NULL, 如果等于|view|.
        //
        // 当视图从RootView移除时调用, 确保目标视图不为非法值.
        void ResetTargetViewIfEquals(View* view);

        // 拖拽过程中当鼠标拖到根视图上时触发. 方法为目标视图返回DragDropTypes类型的
        // 组合. 如果没有视图接受拖放, 返回DRAG_NONE.
        DWORD OnDragOver(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

        // 拖拽过程中当鼠标拖到根视图以外时触发.
        void OnDragLeave();

        // 拖拽过程中当鼠标拖放到根视图时触发. 返回值参见OnDragOver.
        //
        // 注意: 实现在调用本函数之前必须调用OnDragOver, 提供返回值用于
        // drag_operation.
        DWORD OnDrop(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

        // 根据根视图坐标系中给定位置计算拖放的目标视图. 如果鼠标仍在target_view_
        // 上, 不再重复查询CanDrop.
        View* CalculateTargetView(const gfx::Point& root_view_location,
            IDataObject* data_object);

    private:
        // CalculateTargetView的实现. 如果|deepest_view|非空, 会返回RootView最深的
        // 包含|root_view_location|点的子视图.
        View* CalculateTargetViewImpl(const gfx::Point& root_view_location,
            IDataObject* data_object,
            View** deepest_view);

        // 发送正确的拖放通知消息到目标视图. 如果目标视图为空, 什么也不做.
        void NotifyDragEntered(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);
        DWORD NotifyDragOver(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);
        void NotifyDragLeave();

        // 创建本对象的RootView.
        View* root_view_;

        // 接受事件的目标视图.
        View* target_view_;

        // 当前拖放坐标下最深的视图.
        View* deepest_view_;

        DISALLOW_COPY_AND_ASSIGN(DropHelper);
    };

} //namespace view

#endif //__view_drop_helper_h__