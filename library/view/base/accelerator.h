
#ifndef __view_framework_accelerator_h__
#define __view_framework_accelerator_h__

#pragma once

#include "base/string16.h"

#include "../event/event.h"

// Accelerator类描述键盘加速键(或者键盘快捷键).
// Keyboard快捷键通过FocusManager注册. 有拷贝和赋值构造函数, 支持拷贝.
// 定义了<操作符, 可在std::map中使用.

namespace view
{

    // 菜单加速键的跨平台基类. 必须被子类实现.
    class MenuAccelerator
    {
    public:
        MenuAccelerator() : key_code_(VKEY_UNKNOWN), modifiers_(0) {}

        MenuAccelerator(KeyboardCode keycode, int modifiers)
            : key_code_(keycode), modifiers_(modifiers) {}

        MenuAccelerator(const MenuAccelerator& accelerator)
        {
            key_code_ = accelerator.key_code_;
            modifiers_ = accelerator.modifiers_;
        }

        virtual ~MenuAccelerator() {}

        MenuAccelerator& operator=(const MenuAccelerator& accelerator)
        {
            if(this != &accelerator)
            {
                key_code_ = accelerator.key_code_;
                modifiers_ = accelerator.modifiers_;
            }
            return *this;
        }

        // 定义<操作符, 可在std::map中使用.
        bool operator <(const MenuAccelerator& rhs) const
        {
            if(key_code_ != rhs.key_code_)
            {
                return key_code_ < rhs.key_code_;
            }
            return modifiers_ < rhs.modifiers_;
        }

        bool operator ==(const MenuAccelerator& rhs) const
        {
            return (key_code_==rhs.key_code_) && (modifiers_==rhs.modifiers_);
        }

        bool operator !=(const MenuAccelerator& rhs) const
        {
            return !(*this == rhs);
        }

        KeyboardCode GetKeyCode() const
        {
            return key_code_;
        }

        int modifiers() const
        {
            return modifiers_;
        }

    protected:
        // 键码(VK_...).
        KeyboardCode key_code_;

        // Shift/Ctrl/Alt键的状态(平台相关的).
        int modifiers_;
    };

    class Accelerator : public MenuAccelerator
    {
    public:
        Accelerator() : MenuAccelerator() {}

        Accelerator(KeyboardCode keycode, int modifiers)
            : MenuAccelerator(keycode, modifiers) {}

        Accelerator(KeyboardCode keycode, bool shift_pressed,
            bool ctrl_pressed, bool alt_pressed)
        {
            key_code_ = keycode;
            modifiers_ = 0;
            if(shift_pressed)
            {
                modifiers_ |= EF_SHIFT_DOWN;
            }
            if(ctrl_pressed)
            {
                modifiers_ |= EF_CONTROL_DOWN;
            }
            if(alt_pressed)
            {
                modifiers_ |= EF_ALT_DOWN;
            }
        }

        virtual ~Accelerator() {}

        bool IsShiftDown() const
        {
            return (modifiers_&EF_SHIFT_DOWN) == EF_SHIFT_DOWN;
        }

        bool IsCtrlDown() const
        {
            return (modifiers_&EF_CONTROL_DOWN) == EF_CONTROL_DOWN;
        }

        bool IsAltDown() const
        {
            return (modifiers_&EF_ALT_DOWN) == EF_ALT_DOWN;
        }

        // Returns a string with the localized shortcut if any.
        string16 GetShortcutText() const;
    };

    // 想要注册键盘加速键的类需要实现本接口.
    class AcceleratorTarget
    {
    public:
        // 如果加速键被处理, 方法应该返回true.
        virtual bool AcceleratorPressed(const Accelerator& accelerator) = 0;

    protected:
        virtual ~AcceleratorTarget() {}
    };

} //namespace view

#endif //__view_framework_accelerator_h__