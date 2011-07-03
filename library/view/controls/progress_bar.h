
#ifndef __view_progress_bar_h__
#define __view_progress_bar_h__

#pragma once

#include <string>

#include "view/view.h"

namespace gfx
{
    class Canvas;
    class Point;
    class Size;
}

namespace view
{

    /////////////////////////////////////////////////////////////////////////////
    //
    // ProgressBar class
    //
    // A progress bar is a control that indicates progress visually.
    //
    /////////////////////////////////////////////////////////////////////////////

    class ProgressBar : public View
    {
    public:
        ProgressBar();
        virtual ~ProgressBar();

        // Overridden to return preferred size of the progress bar.
        virtual gfx::Size GetPreferredSize();

        // Returns view/ProgressBar.
        virtual std::string GetClassName() const;

        // Overridden to paint
        virtual void OnPaint(gfx::Canvas* canvas);

        // Set and get the progress bar progress in range [0, kMaxProgress].
        virtual void SetProgress(int progress);
        virtual int GetProgress() const;
        // Add progress to current.
        virtual void AddProgress(int tick);

        // Sets the tooltip text.  Default behavior for a progress bar is to show
        // no tooltip on mouse hover. Calling this lets you set a custom tooltip.
        // To revert to default behavior, call this with an empty string.
        virtual void SetTooltipText(const std::wstring& tooltip_text);

        // Gets the tooltip text if has been specified with SetTooltipText().
        virtual bool GetTooltipText(const gfx::Point& p, std::wstring* tooltip);

        // Sets the enabled state.
        virtual void OnEnabledChanged();

        // Accessibility accessors, overridden from View.
        virtual void GetAccessibleState(ui::AccessibleViewState* state);

        // Maximum value of progress.
        static const int kMaxProgress;

    private:
        // Progress in range [0, kMaxProgress].
        int progress_;

        // Tooltip text.
        string16 tooltip_text_;

        // The view class name.
        static const char kViewClassName[];

        DISALLOW_COPY_AND_ASSIGN(ProgressBar);
    };

} //namespace view

#endif //__view_progress_bar_h__