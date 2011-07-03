
#ifndef __view_native_menu_host_h__
#define __view_native_menu_host_h__

namespace gfx
{
    class Rect;
}

namespace view
{

    class NativeWidget;
    namespace internal
    {
        class NativeMenuHostDelegate;
    }

    class NativeMenuHost
    {
    public:
        virtual ~NativeMenuHost() {}

        static NativeMenuHost* CreateNativeMenuHost(
            internal::NativeMenuHostDelegate* delegate);

        // Starts capturing input events.
        virtual void StartCapturing() = 0;

        virtual NativeWidget* AsNativeWidget() = 0;
    };

} //namespace view

#endif //__view_native_menu_host_h__