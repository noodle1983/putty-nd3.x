
#ifndef __view_accelerator_h__
#define __view_accelerator_h__

#pragma once

#include "base/string16.h"

#include "ui_base/keycodes/keyboard_codes_win.h"
#include "ui_base/models/accelerator.h"

#include "event/event.h"

// Accelerator类描述键盘加速键(或者键盘快捷键).
// Keyboard快捷键通过FocusManager注册. 有拷贝和赋值构造函数, 支持拷贝.
// 定义了<操作符, 可在std::map中使用.

namespace view
{

    class Accelerator : public ui::Accelerator
    {
    public:
        Accelerator() : ui::Accelerator() {}

        Accelerator(ui::KeyboardCode keycode, int modifiers)
            : ui::Accelerator(keycode, modifiers) {}

        Accelerator(ui::KeyboardCode keycode, bool shift_pressed,
            bool ctrl_pressed, bool alt_pressed)
        {
            key_code_ = keycode;
            modifiers_ = 0;
            if(shift_pressed)
            {
                modifiers_ |= ui::EF_SHIFT_DOWN;
            }
            if(ctrl_pressed)
            {
                modifiers_ |= ui::EF_CONTROL_DOWN;
            }
            if(alt_pressed)
            {
                modifiers_ |= ui::EF_ALT_DOWN;
            }
        }

        virtual ~Accelerator() {}

        bool IsShiftDown() const
        {
            return (modifiers_&ui::EF_SHIFT_DOWN) == ui::EF_SHIFT_DOWN;
        }

        bool IsCtrlDown() const
        {
            return (modifiers_&ui::EF_CONTROL_DOWN) == ui::EF_CONTROL_DOWN;
        }

        bool IsAltDown() const
        {
            return (modifiers_&ui::EF_ALT_DOWN) == ui::EF_ALT_DOWN;
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

#endif //__view_accelerator_h__