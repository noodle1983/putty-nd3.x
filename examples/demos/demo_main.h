
#ifndef __demo_main_h__
#define __demo_main_h__

#pragma once

#include "view/controls/tabbed_pane/tabbed_pane_listener.h"
#include "view/widget/widget_delegate.h"

class StatusIcon;
class StatusTray;

namespace ui
{
    class MenuModel;
}

namespace view
{
    class Label;
    class View;
}

class DemoMain : public view::WidgetDelegate,
    public view::TabbedPaneListener
{
public:
    DemoMain();
    virtual ~DemoMain();

    void DisplayBalloon();
    void SetTrayIconContextMenu(ui::MenuModel* menu);

    // Overridden from view::WidgetDelegate:
    virtual bool CanResize() const;
    virtual std::wstring GetWindowTitle() const;
    virtual view::View* GetContentsView();
    virtual void WindowClosing();
    virtual view::Widget* GetWidget();
    virtual const view::Widget* GetWidget() const;

    // Overridden from view::TabbedPaneListener:
    virtual void TabSelectedAt(int index);

    void SetStatus(const std::wstring& status);
    void Run();

private:
    view::View* contents_;

    view::Label* status_label_;

    scoped_ptr<StatusTray> status_tray_;
    StatusIcon* main_status_icon_;

    DISALLOW_COPY_AND_ASSIGN(DemoMain);
};

#endif //__demo_main_h__