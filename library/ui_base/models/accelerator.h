
#ifndef __ui_base_accelerator_h__
#define __ui_base_accelerator_h__

#pragma once

#include "ui_base/keycodes/keyboard_codes_win.h"

namespace ui
{

    // This is a cross-platform base class for accelerator keys used in menus. It is
    // meant to be subclassed for concrete toolkit implementations.
    class Accelerator
    {
    public:
        Accelerator() : key_code_(VKEY_UNKNOWN), modifiers_(0) {}

        Accelerator(KeyboardCode keycode, int modifiers)
            : key_code_(keycode), modifiers_(modifiers) {}

        Accelerator(const Accelerator& accelerator)
        {
            key_code_ = accelerator.key_code_;
            modifiers_ = accelerator.modifiers_;
        }

        virtual ~Accelerator() {}

        Accelerator& operator=(const Accelerator& accelerator)
        {
            if(this != &accelerator)
            {
                key_code_ = accelerator.key_code_;
                modifiers_ = accelerator.modifiers_;
            }
            return *this;
        }

        // We define the < operator so that the KeyboardShortcut can be used as a key
        // in a std::map.
        bool operator <(const Accelerator& rhs) const
        {
            if(key_code_ != rhs.key_code_)
            {
                return key_code_ < rhs.key_code_;
            }
            return modifiers_ < rhs.modifiers_;
        }

        bool operator ==(const Accelerator& rhs) const
        {
            return (key_code_==rhs.key_code_) && (modifiers_==rhs.modifiers_);
        }

        bool operator !=(const Accelerator& rhs) const
        {
            return !(*this == rhs);
        }

        KeyboardCode key_code() const { return key_code_; }

        int modifiers() const { return modifiers_; }

    protected:
        // The keycode (VK_...).
        KeyboardCode key_code_;

        // The state of the Shift/Ctrl/Alt keys (platform-dependent).
        int modifiers_;
    };

    // Since acclerator code is one of the few things that can't be cross platform
    // in the chrome UI, separate out just the GetAcceleratorForCommandId() from
    // the menu delegates.
    class AcceleratorProvider
    {
    public:
        // Gets the accelerator for the specified command id. Returns true if the
        // command id has a valid accelerator, false otherwise.
        virtual bool GetAcceleratorForCommandId(int command_id,
            Accelerator* accelerator) = 0;

    protected:
        virtual ~AcceleratorProvider() {}
    };

} //namespace ui

#endif //__ui_base_accelerator_h__