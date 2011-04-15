
#ifndef __view_native_menu_host_h__
#define __view_native_menu_host_h__

namespace gfx
{
    class Rect;
}

namespace view
{
    class NativeMenuHostDelegate;
    class NativeWidget;

    class NativeMenuHost
    {
    public:
        virtual ~NativeMenuHost() {}

        static NativeMenuHost* CreateNativeMenuHost(
            NativeMenuHostDelegate* delegate);

        // Initializes and shows the MenuHost.
        virtual void InitMenuHost(HWND parent,
            const gfx::Rect& bounds) = 0;

        // Starts capturing input events.
        virtual void StartCapturing() = 0;

        virtual NativeWidget* AsNativeWidget() = 0;
    };

} //namespace view

#endif //__view_native_menu_host_h__