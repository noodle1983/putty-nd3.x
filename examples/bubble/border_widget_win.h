
#ifndef __border_widget_win_h__
#define __border_widget_win_h__

#pragma once

#include "view/widget/native_widget_win.h"

#include "bubble_border.h"

class BorderContent;

// This is a window that surrounds the info bubble and paints the margin and
// border.  It is a separate window so that it can be a layered window, so that
// we can use >1-bit alpha shadow images on the borders, which look nicer than
// the Windows CS_DROPSHADOW shadows.  The info bubble window itself cannot be a
// layered window because that prevents it from hosting native child controls.
class BorderWidgetWin : public view::NativeWidgetWin
{
public:
    BorderWidgetWin();
    virtual ~BorderWidgetWin() {}

    // Initializes the BrowserWidget making |owner| its owning window.
    void InitBorderWidgetWin(BorderContent* border_content, HWND owner);

    // Given the size of the contained content (without margins), and the rect
    // (in screen coordinates) to point to, sets the border window positions and
    // sizes the border window and returns the bounds (in screen coordinates) the
    // content should use. |arrow_location| is prefered arrow location,
    // the function tries to preserve the location and direction, in case of RTL
    // arrow location is mirrored.
    virtual gfx::Rect SizeAndGetBounds(const gfx::Rect& position_relative_to,
        BubbleBorder::ArrowLocation arrow_location,
        const gfx::Size& content_size);

    // Simple accessors.
    BorderContent* border_content() { return border_content_; }

protected:
    BorderContent* border_content_;

private:
    // Overridden from NativeWidgetWin:
    virtual LRESULT OnMouseActivate(UINT message,
        WPARAM w_param,
        LPARAM l_param);

    DISALLOW_COPY_AND_ASSIGN(BorderWidgetWin);
};

#endif //__border_widget_win_h__