
#ifndef __browser_root_view_h__
#define __browser_root_view_h__

#pragma once

#include "view/widget/root_view.h"

namespace ui
{
    class OSExchangeData;
}

class AbstractTabStripView;
class BrowserView;
class Url;

// RootView implementation used by BrowserFrame. This forwards drop events to
// the TabStrip. Visually the tabstrip extends to the top of the frame, but in
// actually it doesn't. The tabstrip is only as high as a tab. To enable
// dropping above the tabstrip BrowserRootView forwards drop events to the
// TabStrip.
class BrowserRootView : public view::internal::RootView
{
public:
    // Internal class name.
    static const char kViewClassName[];

    // You must call set_tabstrip before this class will accept drops.
    BrowserRootView(BrowserView* browser_view, view::Widget* widget);

    // Overridden from view::View:
    virtual bool GetDropFormats(int* formats,
        std::set<ui::OSExchangeData::CustomFormat>* custom_formats);
    virtual bool AreDropTypesRequired();
    virtual bool CanDrop(const ui::OSExchangeData& data);
    virtual void OnDragEntered(const view::DropTargetEvent& event);
    virtual int OnDragUpdated(const view::DropTargetEvent& event);
    virtual void OnDragExited();
    virtual int OnPerformDrop(const view::DropTargetEvent& event);
    virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;
    virtual std::string GetClassName() const OVERRIDE;

private:
    // Returns true if the event should be forwarded to the tabstrip.
    bool ShouldForwardToTabStrip(const view::DropTargetEvent& event);

    // Converts the event from the hosts coordinate system to the tabstrips
    // coordinate system.
    view::DropTargetEvent* MapEventToTabStrip(
        const view::DropTargetEvent& event,
        const ui::OSExchangeData& data);

    inline AbstractTabStripView* tabstrip() const;

    // Returns true if |data| has string contents and the user can "paste and go".
    // If |url| is non-NULL and the user can "paste and go", |url| is set to the
    // desired destination.
    bool GetPasteAndGoURL(const ui::OSExchangeData& data, Url* url);

    // The BrowserView.
    BrowserView* browser_view_;

    // If true, drag and drop events are being forwarded to the tab strip.
    // This is used to determine when to send OnDragEntered and OnDragExited
    // to the tab strip.
    bool forwarding_to_tab_strip_;

    DISALLOW_COPY_AND_ASSIGN(BrowserRootView);
};

#endif //__browser_root_view_h__