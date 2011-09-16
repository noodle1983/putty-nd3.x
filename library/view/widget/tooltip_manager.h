
#ifndef __view_tooltip_manager_h__
#define __view_tooltip_manager_h__

#pragma once

#include <string>

#include "base/basic_types.h"
#include "base/string16.h"

namespace gfx
{
    class Font;
}

namespace view
{

    class View;

    // TooltipManager takes care of the wiring to support tooltips for Views. You
    // almost never need to interact directly with TooltipManager, rather look to
    // the various tooltip methods on View.
    class TooltipManager
    {
    public:
        // Returns the height of tooltips. This should only be invoked from within
        // GetTooltipTextOrigin.
        static int GetTooltipHeight();

        // Returns the default font used by tooltips.
        static gfx::Font GetDefaultFont();

        // Returns the separator for lines of text in a tooltip.
        static const std::wstring& GetLineSeparator();

        // Returns the maximum width of the tooltip. |x| and |y| give the location
        // the tooltip is to be displayed on in screen coordinates.
        static int GetMaxWidth(int x, int y);

        TooltipManager() {}
        virtual ~TooltipManager() {}

        // Notification that the view hierarchy has changed in some way.
        virtual void UpdateTooltip() = 0;

        // Invoked when the tooltip text changes for the specified views.
        virtual void TooltipTextChanged(View* view) = 0;

        // Invoked when toolbar icon gets focus.
        virtual void ShowKeyboardTooltip(View* view) = 0;

        // Invoked when toolbar loses focus.
        virtual void HideKeyboardTooltip() = 0;

    protected:
        // Trims the tooltip to fit, setting |text| to the clipped result,
        // |max_width| to the width (in pixels) of the clipped text and |line_count|
        // to the number of lines of text in the tooltip. |x| and |y| give the
        // location of the tooltip in screen coordinates.
        static void TrimTooltipToFit(string16* text,
            int* max_width,
            int* line_count,
            int x,
            int y);
    };

} //namespace view

#endif //__view_tooltip_manager_h__