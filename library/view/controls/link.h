
#ifndef __view_link_h__
#define __view_link_h__

#pragma once

#include "label.h"

namespace view
{

    class LinkListener;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Link class
    //
    // A Link is a label subclass that looks like an HTML link. It has a
    // controller which is notified when a click occurs.
    //
    ////////////////////////////////////////////////////////////////////////////////
    class Link : public Label
    {
    public:
        Link();
        explicit Link(const std::wstring& title);
        virtual ~Link();

        const LinkListener* listener() { return listener_; }
        void set_listener(LinkListener* listener) { listener_ = listener; }

        // Overridden from View:
        virtual void OnEnabledChanged();
        virtual std::string GetClassName() const;
        virtual bool OnSetCursor(const gfx::Point& p);
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual void OnMouseCaptureLost();
        virtual bool OnKeyPressed(const KeyEvent& e);
        virtual bool SkipDefaultKeyEventProcessing(const KeyEvent& e);
        virtual void GetAccessibleState(ui::AccessibleViewState* state);

        // Overridden from Label:
        virtual void SetFont(const gfx::Font& font);

        void SetHighlightedColor(const SkColor& color);
        void SetDisabledColor(const SkColor& color);
        void SetNormalColor(const SkColor& color);

        // If you'll be displaying the link over some non-system background color,
        // call this with the relevant color and the link will auto-set its colors to
        // be readable.
        void MakeReadableOverBackgroundColor(const SkColor& color);

        static const char kViewClassName[];

    private:
        // A highlighted link is clicked.
        void SetHighlighted(bool f);

        // Make sure the label style matched the current state.
        void ValidateStyle();

        void Init();

        LinkListener* listener_;

        // Whether the link is currently highlighted.
        bool highlighted_;

        // The color when the link is highlighted.
        SkColor highlighted_color_;

        // The color when the link is disabled.
        SkColor disabled_color_;

        // The color when the link is neither highlighted nor disabled.
        SkColor normal_color_;

        DISALLOW_COPY_AND_ASSIGN(Link);
    };

} //namespace view

#endif //__view_link_h__