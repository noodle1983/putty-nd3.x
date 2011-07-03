
#ifndef __ui_base_drop_target_h__
#define __ui_base_drop_target_h__

#pragma once

#include <objidl.h>

#include "base/memory/ref_counted.h"

// Windows接口.
struct IDropTargetHelper;

namespace ui
{

    // DropTarget实现繁琐的拖拽过程的管理. 类在具现时, 派生类一般都需要覆盖
    // 各种OnXXX方法.
    //
    // 因为DropTarget使用了引用计数, 所以不要直接删除它, 而是放在scoped_refptr
    // 中. 记住在窗口删除前调用RevokeDragDrop(m_hWnd).
    //
    // 在STA中使用这个类, 它不是线程安全的.
    class DropTarget : public IDropTarget
    {
    public:
        // 创建一个和给定HWND关联的DropTarget.
        explicit DropTarget(HWND hwnd);
        virtual ~DropTarget();

        // 当suspended设置为|true|时, 停止处理本窗口初始化的拖放通知.
        bool suspended() const { return suspended_; }
        void set_suspended(bool suspended) { suspended_ = suspended; }

        // IDropTarget实现:
        HRESULT __stdcall DragEnter(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD* effect);
        HRESULT __stdcall DragOver(DWORD key_state,
            POINTL cursor_position,
            DWORD* effect);
        HRESULT __stdcall DragLeave();
        HRESULT __stdcall Drop(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD* effect);

        // IUnknown实现:
        HRESULT __stdcall QueryInterface(const IID& iid, void** object);
        ULONG __stdcall AddRef();
        ULONG __stdcall Release();

    protected:
        // 返回宿主HWND.
        HWND GetHWND() { return hwnd_; }

        // 拖拽过程中当鼠标第一次悬停在窗口时触发. 返回值应该是支持的拖放操作组合:
        // DROPEFFECT_NONE、DROPEFFECT_COPY、DROPEFFECT_LINK和/或DROPEFFECT_MOVE.
        virtual DWORD OnDragEnter(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

        // 拖拽过程中当鼠标移动悬停在窗口时触发. 返回值应该是支持的拖放操作组合:
        // DROPEFFECT_NONE、DROPEFFECT_COPY、DROPEFFECT_LINK和/或DROPEFFECT_MOVE.
        virtual DWORD OnDragOver(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

        // 拖拽过程中当鼠标移出窗口外时触发.
        virtual void OnDragLeave(IDataObject* data_object);

        // 拖放到窗口上时触发. 返回采取的操作.
        virtual DWORD OnDrop(IDataObject* data_object,
            DWORD key_state,
            POINTL cursor_position,
            DWORD effect);

    private:
        // 返回缓冲的拖放辅助器, 需要时创建一个. 返回值不用addrefed. 如果对象创建
        // 失败会返回NULL.
        static IDropTargetHelper* DropHelper();

        // 当前悬停在拖放对象上的数据对象.
        scoped_refptr<IDataObject> current_data_object_;

        // 辅助对象用于提供鼠标悬停在内容区域时的拖拽图像.
        //
        // 不要直接访问! 使用DropHelper(), 它会在不存在时懒创建对象. 对象的创建可能
        // 花费几十毫秒, 我们不想堵塞窗口打开过程, 尤其是拖拽可能经常不会用到. 但是
        // 我们在它实际用到时会强制创建.
        static IDropTargetHelper* cached_drop_target_helper_;

        // 源窗口局部, 用于确定鼠标事件时的坐标点, 发生到渲染器通知各种拖放状态.
        HWND hwnd_;

        // 当前是否停止处理本窗口初始化的拖放通知.
        bool suspended_;

        LONG ref_count_;

        DISALLOW_COPY_AND_ASSIGN(DropTarget);
    };

} //namespace ui

#endif //__ui_base_drop_target_h__