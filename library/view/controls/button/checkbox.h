
#ifndef __view_checkbox_h__
#define __view_checkbox_h__

#pragma once

#include <string>

#include "native_button.h"
#include "text_button.h"

namespace view
{

    class Label;

    // A NativeButton subclass representing a checkbox.
    class NativeCheckbox : public NativeButton
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        NativeCheckbox();
        explicit NativeCheckbox(const std::wstring& label);
        virtual ~NativeCheckbox();

        // Returns the indentation of the text from the left edge of the view.
        static int GetTextIndent();

        // Sets a listener for this checkbox. Checkboxes aren't required to have them
        // since their state can be read independently of them being toggled.
        void set_listener(ButtonListener* listener) { listener_ = listener; }

        // Sets whether or not the checkbox label should wrap multiple lines of text.
        // If true, long lines are wrapped, and this is reflected in the preferred
        // size returned by GetPreferredSize. If false, text that will not fit within
        // the available bounds for the label will be cropped.
        void SetMultiLine(bool multiline);

        // Sets/Gets whether or not the checkbox is checked.
        virtual void SetChecked(bool checked);
        bool checked() const { return checked_; }

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual int GetHeightForWidth(int w);
        virtual void OnEnabledChanged();
        virtual void Layout();
        virtual std::string GetClassName() const;
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual void OnMouseCaptureLost();
        virtual void OnMouseMoved(const MouseEvent& event);
        virtual void OnMouseEntered(const MouseEvent& event);
        virtual void OnMouseExited(const MouseEvent& event);
        virtual void GetAccessibleState(ui::AccessibleViewState* state);

        // Overridden from NativeButton:
        virtual void SetLabel(const std::wstring& label);

    protected:
        // Returns true if the event (in NativeCheckbox coordinates) is within the bounds of
        // the label.
        bool HitTestLabel(const MouseEvent& event);

        // Overridden from View:
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);
        virtual void OnFocus();
        virtual void OnBlur();

        // Overridden from NativeButton:
        virtual NativeButtonWrapper* CreateWrapper();
        virtual void InitBorder();

    private:
        // Called from the constructor to create and configure the checkbox label.
        void Init(const std::wstring& label_text);

        // The checkbox's label. We may not be able to use the OS version on some
        // platforms because of transparency and sizing issues.
        Label* label_;

        // True if the checkbox is checked.
        bool checked_;

        DISALLOW_COPY_AND_ASSIGN(NativeCheckbox);
    };

    // A native themed class representing a checkbox.  This class does not use
    // platform specific objects to replicate the native platforms looks and feel.
    class Checkbox : public TextButtonBase
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        explicit Checkbox(const std::wstring& label);
        virtual ~Checkbox();

        // Sets a listener for this checkbox. Checkboxes aren't required to have them
        // since their state can be read independently of them being toggled.
        void set_listener(ButtonListener* listener) { listener_ = listener; }

        // Sets/Gets whether or not the checkbox is checked.
        virtual void SetChecked(bool checked);
        bool checked() const { return checked_; }

        void SetLabel(const std::wstring& label)
        {
            SetText(label);
        }

    protected:
        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual std::string GetClassName() const;
        virtual void GetAccessibleState(ui::AccessibleViewState* state);
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);

    private:
        // Overridden from Button:
        virtual void NotifyClick(const Event& event);

        // Overridden from TextButtonBase:
        virtual gfx::NativeTheme::Part GetThemePart() const;
        virtual gfx::Rect GetThemePaintRect() const;
        virtual void GetExtraParams(
            gfx::NativeTheme::ExtraParams* params) const;
        virtual gfx::Rect GetTextBounds() const;

        // True if the checkbox is checked.
        bool checked_;

        DISALLOW_COPY_AND_ASSIGN(Checkbox);
    };

} //namespace view

#endif //__view_checkbox_h__