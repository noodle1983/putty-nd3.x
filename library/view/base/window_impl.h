
#ifndef __view_window_impl_h__
#define __view_window_impl_h__

#pragma once

#include <windows.h>

#include <string>

#include "base/logging.h"

#include "gfx/rect.h"

namespace view
{

    // 使用消息映射的类需要实现本接口. ProcessWindowMessage在宏
    // VIEW_BEGIN_MESSAGE_MAP_EX中实现.
    class MessageMapInterface
    {
    public:
        // 从window的消息队列中处理一条消息.
        virtual BOOL ProcessWindowMessage(HWND window,
            UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT& result,
            DWORD msg_mad_id = 0) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //
    // WindowImpl
    //  类封装了HWND创建和销毁细节, 且定义了所有Windows使用的函数过程.
    //
    ///////////////////////////////////////////////////////////////////////////////
    class WindowImpl : public MessageMapInterface
    {
    public:
        WindowImpl();
        virtual ~WindowImpl();

        // 初始化父窗口和初始大小.
        void Init(HWND parent, const gfx::Rect& bounds);

        // 返回缺省窗口图标, 在没有指定时使用.
        virtual HICON GetDefaultWindowIcon() const;

        // 返回Window关联的HWND.
        HWND hwnd() const { return hwnd_; }

        // 设置窗口风格. 只有在窗口创建的时候使用. 也就是说在Init调用之后再设置,
        // 没有任何效果.
        void set_window_style(DWORD style) { window_style_ = style; }
        DWORD window_style() const { return window_style_; }

        // 设置窗口的扩展风格. 参见|set_window_style|的注释.
        void set_window_ex_style(DWORD style) { window_ex_style_ = style; }
        DWORD window_ex_style() const { return window_ex_style_; }

        // 设置类使用的风格. 缺省是CS_DBLCLKS.
        void set_initial_class_style(UINT class_style)
        {
            // 类名是动态创建的, 所以不要注册为全局的!
            DCHECK_EQ((class_style&CS_GLOBALCLASS), 0u);
            class_style_ = class_style;
        }
        UINT initial_class_style() const { return class_style_; }

        // 如果|hwnd|是一个WindowImpl, 返回true.
        static bool IsWindowImpl(HWND hwnd);

    protected:
        // 处理本对象的WndProc回调.
        virtual LRESULT OnWndProc(UINT message, WPARAM w_param, LPARAM l_param);

    private:
        friend class ClassRegistrar;

        // 所有Windows使用的窗口过程.
        static LRESULT CALLBACK WndProc(HWND window, UINT message,
            WPARAM w_param, LPARAM l_param);

        // 在创建HWND时用来获得窗口的类名. 函数会根据需要注册窗口类.
        std::wstring GetWindowClassName();

        // 所有WidgetWin注册的类名前缀.
        static const wchar_t* const kBaseClassName;

        // 窗口创建时使用的窗口风格.
        DWORD window_style_;

        // 窗口创建时使用的窗口扩展风格.
        DWORD window_ex_style_;

        // 窗口类风格.
        UINT class_style_;

        // 窗口句柄.
        HWND hwnd_;

        DISALLOW_COPY_AND_ASSIGN(WindowImpl);
    };

} //namespace view

#endif //__view_window_impl_h__