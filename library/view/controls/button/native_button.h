
#ifndef __view_native_button_h__
#define __view_native_button_h__

#pragma once

#include "gfx/font.h"

#include "button.h"
#include "native_button_wrapper.h"

namespace gfx
{
    class Font;
}

namespace view
{

    class NativeButtonBase : public Button
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        explicit NativeButtonBase(ButtonListener* listener);
        NativeButtonBase(ButtonListener* listener, const std::wstring& label);
        virtual ~NativeButtonBase();

        // Sets/Gets the text to be used as the button's label.
        virtual void SetLabel(const std::wstring& label);
        std::wstring label() const { return label_; }

        // Sets the font to be used when displaying the button's label.
        void set_font(const gfx::Font& font) { font_ = font; }
        const gfx::Font& font() const { return font_; }

        // Sets/Gets whether or not the button appears and behaves as the default
        // button in its current context.
        void SetIsDefault(bool default_button);
        bool is_default() const { return is_default_; }

        // Sets/Gets whether or not pressing this button requires elevation.
        void SetNeedElevation(bool need_elevation);
        bool need_elevation() const { return need_elevation_; }

        void set_ignore_minimum_size(bool ignore_minimum_size)
        {
            ignore_minimum_size_ = ignore_minimum_size;
        }

        // Called by the wrapper when the actual wrapped native button was pressed.
        void ButtonPressed();

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void SetEnabled(bool flag);
        virtual void OnFocus();
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);

    protected:
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual std::string GetClassName() const;
        virtual bool AcceleratorPressed(const Accelerator& accelerator);

        // Create the button wrapper and returns it. Ownership of the returned
        // value is passed to the caller.
        //
        // This can be overridden by subclass to create a wrapper of a particular
        // type. See NativeButtonWrapper interface for types.
        virtual NativeButtonWrapper* CreateWrapper();

        // Sets a border to the button. Override to set a different border or to not
        // set one (the default is 0,8,0,8 for push buttons).
        virtual void InitBorder();

        // The object that actually implements the native button.
        NativeButtonWrapper* native_wrapper_;

    private:
        // The button label.
        string16 label_;

        // True if the button is the default button in its context.
        bool is_default_;

        // True if this button requires elevation (or sudo). This flag is currently
        // used for adding a shield icon on Windows Vista or later.
        bool need_elevation_;

        // The font used to render the button label.
        gfx::Font font_;

        // True if the button should ignore the minimum size for the platform. Default
        // is false. Set to true to create narrower buttons.
        bool ignore_minimum_size_;

        DISALLOW_COPY_AND_ASSIGN(NativeButtonBase);
    };

    // TODO(saintlou): Windows and Clang do not like typedef, it
    // chokes in other modules that have a forward declaration for
    // NativeButton
    class NativeButton : public NativeButtonBase
    {
    public:
        explicit NativeButton(ButtonListener* listener);
        NativeButton(ButtonListener* listener, const std::wstring& label);
        virtual ~NativeButton();

    private:
        DISALLOW_COPY_AND_ASSIGN(NativeButton);
    };

} //namespace view

#endif //__view_native_button_h__