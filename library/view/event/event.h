
#ifndef __view_event_h__
#define __view_event_h__

#pragma once

#include "base/time.h"

#include "ui_gfx/point.h"

#include "ui_base/events.h"
#include "ui_base/keycodes/keyboard_codes_win.h"

namespace ui
{
    class OSExchangeData;
}

namespace view
{

    class View;

    namespace internal
    {
        class NativeWidgetView;
        class RootView;
    }

    bool IsClientMouseEvent(const MSG& native_event);
    bool IsNonClientMouseEvent(const MSG& native_event);

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Event类
    //
    // 输入事件的封装, 在view树中传递. 事件包含类型、一些标记和时间戳.
    // 每个主要的事件都有对应的子类.
    // 事件是不可变的, 但是支持拷贝.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class Event
    {
    public:
        const MSG& native_event() const { return native_event_; }
        ui::EventType type() const { return type_; }
        const base::Time& time_stamp() const { return time_stamp_; }
        int flags() const { return flags_; }
        void set_flags(int flags) { flags_ = flags; }

        // 返回shift键是否按下.
        bool IsShiftDown() const
        {
            return (flags_ & ui::EF_SHIFT_DOWN) != 0;
        }

        // 返回control键是否按下.
        bool IsControlDown() const
        {
            return (flags_ & ui::EF_CONTROL_DOWN) != 0;
        }

        // 返回caps键是否锁定.
        bool IsCapsLockDown() const
        {
            return (flags_ & ui::EF_CAPS_LOCK_DOWN) != 0;
        }

        // 返回alt键是否按下.
        bool IsAltDown() const
        {
            return (flags_ & ui::EF_ALT_DOWN) != 0;
        }

        bool IsMouseEvent() const
        {
            return type_==ui::ET_MOUSE_PRESSED ||
                type_==ui::ET_MOUSE_DRAGGED ||
                type_==ui::ET_MOUSE_RELEASED ||
                type_==ui::ET_MOUSE_MOVED ||
                type_==ui::ET_MOUSE_ENTERED ||
                type_==ui::ET_MOUSE_EXITED ||
                type_==ui::ET_MOUSEWHEEL;
        }

        // 以windows的标记格式返回EventFlags.
        int GetWindowsFlags() const;

    protected:
        Event(ui::EventType type, int flags);
        Event(MSG native_event, ui::EventType type, int flags);

        Event(const Event& model)
            : native_event_(model.native_event()),
            type_(model.type()),
            time_stamp_(model.time_stamp()),
            flags_(model.flags()) {}

        void set_type(ui::EventType type) { type_ = type; }

    private:
        void operator=(const Event&);

        // 初始化类成员变量.
        void Init();
        void InitWithNativeEvent(MSG native_event);

        MSG native_event_;
        ui::EventType type_;
        base::Time time_stamp_;
        int flags_;
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // LocatedEvent类
    //
    // 事件用于定位屏幕上的指定位置.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class LocatedEvent : public Event
    {
    public:
        int x() const { return location_.x(); }
        int y() const { return location_.y(); }
        const gfx::Point& location() const { return location_; }

    protected:
        explicit LocatedEvent(MSG native_event);

        LocatedEvent(ui::EventType type, const gfx::Point& location, int flags);

        // 创建一个与model相同的LocatedEvent. 如果提供了source/target视图, model位置
        // 从'source'坐标系统转换到'target'坐标系统.
        LocatedEvent(const LocatedEvent& model, View* source, View* target);

        // 这个构造函数把事件位置从widget坐标转换到RootView坐标.
        LocatedEvent(const LocatedEvent& model, View* root);

        gfx::Point location_;
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // MouseEvent类
    //
    // 鼠标相关的输入事件.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class MouseEvent : public LocatedEvent
    {
    public:
        explicit MouseEvent(MSG native_event);

        // Create a new MouseEvent which is identical to the provided model.
        // If source / target views are provided, the model location will be converted
        // from |source| coordinate system to |target| coordinate system.
        MouseEvent(const MouseEvent& model, View* source, View* target);

        // TODO(msw): Kill this legacy constructor when we update uses.
        // Create a new mouse event
        MouseEvent(ui::EventType type, int x, int y, int flags)
            : LocatedEvent(type, gfx::Point(x, y), flags) {}

        // 便于快速判断哪个键按下.
        bool IsOnlyLeftMouseButton() const
        {
            return (flags()&ui::EF_LEFT_BUTTON_DOWN) &&
                !(flags()&(ui::EF_MIDDLE_BUTTON_DOWN|ui::EF_RIGHT_BUTTON_DOWN));
        }

        bool IsLeftMouseButton() const
        {
            return (flags() & ui::EF_LEFT_BUTTON_DOWN) != 0;
        }

        bool IsOnlyMiddleMouseButton() const
        {
            return (flags()&ui::EF_MIDDLE_BUTTON_DOWN) &&
                !(flags()&(ui::EF_LEFT_BUTTON_DOWN|ui::EF_RIGHT_BUTTON_DOWN));
        }

        bool IsMiddleMouseButton() const
        {
            return (flags()&ui::EF_MIDDLE_BUTTON_DOWN) != 0;
        }

        bool IsOnlyRightMouseButton() const
        {
            return (flags()&ui::EF_RIGHT_BUTTON_DOWN) &&
                !(flags()&(ui::EF_LEFT_BUTTON_DOWN|ui::EF_MIDDLE_BUTTON_DOWN));
        }

        bool IsRightMouseButton() const
        {
            return (flags()&ui::EF_RIGHT_BUTTON_DOWN) != 0;
        }

    protected:
        MouseEvent(const MouseEvent& model, View* root)
            : LocatedEvent(model, root) {}

    private:
        friend class internal::NativeWidgetView;
        friend class internal::RootView;

        DISALLOW_COPY_AND_ASSIGN(MouseEvent);
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // KeyEvent类
    //
    // 键盘相关的输入事件.
    // 注意: 事件是按键相关的, 而不是输入字符.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class KeyEvent : public Event
    {
    public:
        explicit KeyEvent(MSG native_event);

        // Creates a new KeyEvent synthetically (i.e. not in response to an input
        // event from the host environment). This is typically only used in testing as
        // some metadata obtainable from the underlying native event is not present.
        // It's also used by input methods to fabricate keyboard events.
        KeyEvent(ui::EventType type, ui::KeyboardCode key_code, int event_flags);

        ui::KeyboardCode key_code() const { return key_code_; }

        // These setters allow an I18N virtual keyboard to fabricate a keyboard event
        // which does not have a corresponding ui::KeyboardCode (example: U+00E1 Latin
        // small letter A with acute, U+0410 Cyrillic capital letter A.)
        // GetCharacter() and GetUnmodifiedCharacter() return the character.
        void set_character(uint16 character) { character_ = character; }
        void set_unmodified_character(uint16 unmodified_character)
        {
            unmodified_character_ = unmodified_character;
        }

        // Gets the character generated by this key event. It only supports Unicode
        // BMP characters.
        uint16 GetCharacter() const;

        // Gets the character generated by this key event ignoring concurrently-held
        // modifiers (except shift).
        uint16 GetUnmodifiedCharacter() const;

    private:
        // A helper function to get the character generated by a key event in a
        // platform independent way. It supports control characters as well.
        // It assumes a US keyboard layout is used, so it may only be used when there
        // is no native event or no better way to get the character.
        // For example, if a virtual keyboard implementation can only generate key
        // events with key_code and flags information, then there is no way for us to
        // determine the actual character that should be generate by the key. Because
        // a key_code only represents a physical key on the keyboard, it has nothing
        // to do with the actual character printed on that key. In such case, the only
        // thing we can do is to assume that we are using a US keyboard and get the
        // character according to US keyboard layout definition.
        // If a virtual keyboard implementation wants to support other keyboard
        // layouts, that may generate different text for a certain key than on a US
        // keyboard, a special native event object should be introduced to carry extra
        // information to help determine the correct character.
        // Take XKeyEvent as an example, it contains not only keycode and modifier
        // flags but also group and other extra XKB information to help determine the
        // correct character. That's why we can use XLookupString() function to get
        // the correct text generated by a X key event (See how is GetCharacter()
        // implemented in event_x.cc).
        // TODO(suzhe): define a native event object for virtual keyboard. We may need
        // to take the actual feature requirement into account.
        static uint16 GetCharacterFromKeyCode(ui::KeyboardCode key_code, int flags);

        ui::KeyboardCode key_code_;

        uint16 character_;
        uint16 unmodified_character_;

        DISALLOW_COPY_AND_ASSIGN(KeyEvent);
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // MouseWheelEvent类
    //
    // 生成鼠标滚轮用户事件.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class MouseWheelEvent : public MouseEvent
    {
    public:
        // 详细参见|offset|.
        static const int kWheelDelta;

        explicit MouseWheelEvent(MSG native_event);

        // 滚动量. 是kWheelDelta的倍数.
        int offset() const { return offset_; }

    private:
        friend class internal::NativeWidgetView;
        friend class internal::RootView;

        MouseWheelEvent(const MouseWheelEvent& model, View* root)
            : MouseEvent(model, root), offset_(model.offset_) {}

        int offset_;

        DISALLOW_COPY_AND_ASSIGN(MouseWheelEvent);
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // DropTargetEvent类
    //
    // 在拖拽操作时发送到鼠标悬停的视图.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class DropTargetEvent : public LocatedEvent
    {
    public:
        DropTargetEvent(const ui::OSExchangeData& data,
            int x,
            int y,
            int source_operations)
            : LocatedEvent(ui::ET_DROP_TARGET_EVENT, gfx::Point(x, y), 0),
            data_(data),
            source_operations_(source_operations) {}

        const ui::OSExchangeData& data() const { return data_; }
        int source_operations() const { return source_operations_; }

    private:
        // 拖拽过程中关联的数据.
        const ui::OSExchangeData& data_;

        // 支持拖拽源的DragDropTypes::DragOperation操作的位掩码.
        int source_operations_;

        DISALLOW_COPY_AND_ASSIGN(DropTargetEvent);
    };

} //namespace view

#endif //__view_event_h__