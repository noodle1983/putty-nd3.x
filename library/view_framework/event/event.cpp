
#include "event.h"

#include <windows.h>

#include "base/logging.h"

#include "../keycodes/keyboard_code_conversion_win.h"
#include "../view/root_view.h"

namespace
{

    // Returns a mask corresponding to the set of modifier keys that are currently
    // pressed. Windows key messages don't come with control key state as parameters
    // as with mouse messages, so we need to explicitly ask for these states.
    int GetKeyStateFlags()
    {
        int flags = 0;
        flags |= (GetKeyState(VK_MENU) & 0x80)? view::EF_ALT_DOWN : 0;
        flags |= (GetKeyState(VK_SHIFT) & 0x80)? view::EF_SHIFT_DOWN : 0;
        flags |= (GetKeyState(VK_CONTROL) & 0x80)? view::EF_CONTROL_DOWN : 0;
        return flags;
    }

    // Convert windows message identifiers to Event types.
    view::EventType EventTypeFromNative(MSG native_event)
    {
        switch(native_event.message)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_CHAR:
            return view::ET_KEY_PRESSED;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            return view::ET_KEY_RELEASED;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_RBUTTONDOWN:
            return view::ET_MOUSE_PRESSED;
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCMBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCRBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONUP:
            return view::ET_MOUSE_RELEASED;
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
            return view::ET_MOUSE_MOVED;
        case WM_MOUSEWHEEL:
            return view::ET_MOUSEWHEEL;
        case WM_MOUSELEAVE:
        case WM_NCMOUSELEAVE:
            return view::ET_MOUSE_EXITED;
        default:
            NOTREACHED();
        }
        return view::ET_UNKNOWN;
    }

    bool IsClientMouseEvent(MSG native_event)
    {
        return native_event.message==WM_MOUSELEAVE ||
            native_event.message==WM_MOUSEHOVER ||
            (native_event.message>=WM_MOUSEFIRST &&
            native_event.message<=WM_MOUSELAST);
    }

    bool IsNonClientMouseEvent(MSG native_event)
    {
        return native_event.message==WM_NCMOUSELEAVE ||
            native_event.message==WM_NCMOUSEHOVER ||
            (native_event.message>=WM_NCMOUSEMOVE &&
            native_event.message<=WM_NCXBUTTONDBLCLK);
    }

    // Get view::Event flags from a native Windows message
    int EventFlagsFromNative(MSG native_event)
    {
        int flags = 0;

        // TODO(msw): ORing the pressed/released button into the flags is _wrong_.
        // It makes it impossible to tell which button was modified when multiple
        // buttons are/were held down. We need to instead put the modified button into
        // a separate member on the MouseEvent, then audit all consumers of
        // MouseEvents to fix them to use the resulting values correctly.
        switch(native_event.message)
        {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
            native_event.wParam |= MK_LBUTTON;
            break;
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
            native_event.wParam |= MK_MBUTTON;
            break;
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
            native_event.wParam |= MK_RBUTTON;
            break;
        }

        // Check if the event occurred in the non-client area.
        if(IsNonClientMouseEvent(native_event))
        {
            flags |= view::EF_IS_NON_CLIENT;
        }

        // Check for double click events.
        switch(native_event.message)
        {
        case WM_NCLBUTTONDBLCLK:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCRBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            flags |= view::EF_IS_DOUBLE_CLICK;
            break;
        }

        UINT win_flags = GET_KEYSTATE_WPARAM(native_event.wParam);
        flags |= (win_flags & MK_CONTROL) ? view::EF_CONTROL_DOWN : 0;
        flags |= (win_flags & MK_SHIFT) ? view::EF_SHIFT_DOWN : 0;
        flags |= (GetKeyState(VK_MENU) < 0) ? view::EF_ALT_DOWN : 0;
        flags |= (win_flags & MK_LBUTTON) ? view::EF_LEFT_BUTTON_DOWN : 0;
        flags |= (win_flags & MK_MBUTTON) ? view::EF_MIDDLE_BUTTON_DOWN : 0;
        flags |= (win_flags & MK_RBUTTON) ? view::EF_RIGHT_BUTTON_DOWN : 0;

        return flags;
    }

}

namespace view
{

    Event::Event(EventType type, int flags) : type_(type),
        time_stamp_(base::Time::NowFromSystemTime()), flags_(flags)
    {
        Init();
    }

    Event::Event(MSG native_event, EventType type, int flags)
        : type_(type),
        time_stamp_(base::Time::NowFromSystemTime()),
        flags_(flags)
    {
        InitWithNativeEvent(native_event);
    }

    int Event::GetWindowsFlags() const
    {
        // TODO: need support for x1/x2.
        int result = 0;
        result |= (flags_&EF_SHIFT_DOWN) ? MK_SHIFT : 0;
        result |= (flags_&EF_CONTROL_DOWN) ? MK_CONTROL : 0;
        result |= (flags_&EF_LEFT_BUTTON_DOWN) ? MK_LBUTTON : 0;
        result |= (flags_&EF_MIDDLE_BUTTON_DOWN) ? MK_MBUTTON : 0;
        result |= (flags_&EF_RIGHT_BUTTON_DOWN) ? MK_RBUTTON : 0;
        return result;
    }

    void Event::Init()
    {
        ZeroMemory(&native_event_, sizeof(native_event_));
    }

    void Event::InitWithNativeEvent(MSG native_event)
    {
        native_event_ = native_event;
    }


    LocatedEvent::LocatedEvent(MSG native_event)
        : Event(native_event, EventTypeFromNative(native_event),
        EventFlagsFromNative(native_event)),
        location_(native_event.pt.x, native_event.pt.y) {}

    LocatedEvent::LocatedEvent(EventType type, const gfx::Point& location,
        int flags) : Event(type, flags), location_(location) {}

    LocatedEvent::LocatedEvent(const LocatedEvent& model, View* source,
        View* target)
        : Event(model),
        location_(model.location_)
    {
        if(target)
        {
            View::ConvertPointToView(source, target, &location_);
        }
    }

    LocatedEvent::LocatedEvent(const LocatedEvent& model, RootView* root)
        : Event(model), location_(model.location_)
    {
        View::ConvertPointFromWidget(root, &location_);
    }


    KeyEvent::KeyEvent(MSG native_event)
        : Event(native_event,
        EventTypeFromNative(native_event),
        GetKeyStateFlags()),
        key_code_(KeyboardCodeForWindowsKeyCode(native_event.wParam)) {}

    KeyEvent::KeyEvent(EventType type, KeyboardCode key_code,
        int event_flags) : Event(type, event_flags), key_code_(key_code) {}


    MouseEvent::MouseEvent(MSG native_event)
        : LocatedEvent(native_event)
    {
        if(IsNonClientMouseEvent(native_event))
        {
            // Non-client message. The position is contained in a POINTS structure in
            // LPARAM, and is in screen coordinates so we have to convert to client.
            POINT native_point = location_.ToPOINT();
            ScreenToClient(native_event.hwnd, &native_point);
            location_ = gfx::Point(native_point);
        }
    }

    MouseEvent::MouseEvent(EventType type,
        View* source,
        View* target,
        const gfx::Point &l,
        int flags)
        : LocatedEvent(MouseEvent(type, l.x(), l.y(), flags), source, target) {}

    MouseEvent::MouseEvent(const MouseEvent& model, View* source, View* target)
        : LocatedEvent(model, source, target) {}


    // This value matches windows WHEEL_DELTA.
    // static
    const int MouseWheelEvent::kWheelDelta = 120;

    MouseWheelEvent::MouseWheelEvent(MSG native_event)
        : MouseEvent(native_event),
        offset_(GET_WHEEL_DELTA_WPARAM(native_event.wParam)) {}

} //namespace view