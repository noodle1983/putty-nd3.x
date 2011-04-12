
#ifndef __wan_chrome_ui_view_frame_browser_root_view_h__
#define __wan_chrome_ui_view_frame_browser_root_view_h__

#pragma once

#include "view/view/root_view.h"

class OSExchangeData;

class BrowserView;

// RootView implementation used by BrowserFrame. This forwards drop events to
// the TabStrip. Visually the tabstrip extends to the top of the frame, but in
// actually it doesn't. The tabstrip is only as high as a tab. To enable
// dropping above the tabstrip BrowserRootView forwards drop events to the
// TabStrip.
class BrowserRootView : public view::RootView
{
public:
    // You must call set_tabstrip before this class will accept drops.
    BrowserRootView(BrowserView* browser_view, view::Widget* widget);

    // Overridden from view::View:
    virtual bool GetDropFormats(int* formats,
        std::set<OSExchangeData::CustomFormat>* custom_formats);
    virtual bool AreDropTypesRequired();
    virtual bool CanDrop(const OSExchangeData& data);
    virtual void OnDragEntered(const view::DropTargetEvent& event);
    virtual int OnDragUpdated(const view::DropTargetEvent& event);
    virtual void OnDragExited();
    virtual int OnPerformDrop(const view::DropTargetEvent& event);
    virtual void GetAccessibleState(AccessibleViewState* state);

private:
    // The BrowserView.
    BrowserView* browser_view_;

    // If true, drag and drop events are being forwarded to the tab strip.
    // This is used to determine when to send OnDragEntered and OnDragExited
    // to the tab strip.
    bool forwarding_to_tab_strip_;

    DISALLOW_COPY_AND_ASSIGN(BrowserRootView);
};

#endif //__wan_chrome_ui_view_frame_browser_root_view_h__