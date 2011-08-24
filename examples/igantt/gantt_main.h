
#ifndef __gantt_main_h__
#define __gantt_main_h__

#pragma once

#include "view/widget/widget_delegate.h"

class GanttMain : public view::WidgetDelegate
{
public:
    GanttMain();
    virtual ~GanttMain();

    // Overridden from view::WidgetDelegate:
    virtual bool CanResize() const;
    virtual bool CanMaximize() const;
    virtual std::wstring GetWindowTitle() const;
    virtual view::View* GetContentsView();
    virtual void WindowClosing();
    virtual view::Widget* GetWidget();
    virtual const view::Widget* GetWidget() const;

    void Run();

private:
    view::View* contents_;

    DISALLOW_COPY_AND_ASSIGN(GanttMain);
};

#endif //__gantt_main_h__