
#ifndef __tab_contents_wrapper_h__
#define __tab_contents_wrapper_h__

#pragma once

#include <string>
#include <vector>

#include "tab_contents.h"

class SkBitmap;

class InfoBarDelegate;
class RenderViewHost;
class TabContentsWrapperDelegate;

// Wraps TabContents and all of its supporting objects in order to control
// their ownership and lifetime, while allowing TabContents to remain generic
// and re-usable in other projects.
// TODO(pinkerton): Eventually, this class will become TabContents as far as
// the browser front-end is concerned, and the current TabContents will be
// renamed to something like WebPage or WebView (ben's suggestions).
class TabContentsWrapper/* : public TabContentsObserver*/
{
public:
    // Takes ownership of |contents|, which must be heap-allocated (as it lives
    // in a scoped_ptr) and can not be NULL.
    explicit TabContentsWrapper(TabContents* contents);
    virtual ~TabContentsWrapper();

    // Initial title assigned to NavigationEntries from Navigate.
    static string16 GetDefaultTitle();

    // Create a TabContentsWrapper with the same state as this one. The returned
    // heap-allocated pointer is owned by the caller.
    TabContentsWrapper* Clone();

    // Captures a snapshot of the page.
    void CaptureSnapshot();

    // Stop this tab rendering in fullscreen mode.
    void ExitFullscreenMode();

    // Helper to retrieve the existing instance that wraps a given TabContents.
    // Returns NULL if there is no such existing instance.
    // NOTE: This is not intended for general use. It is intended for situations
    // like callbacks from content/ where only a TabContents is available. In the
    // general case, please do NOT use this; plumb TabContentsWrapper through the
    // chrome/ code instead of TabContents.
    static TabContentsWrapper* GetCurrentWrapperForContents(
        TabContents* contents);
    static const TabContentsWrapper* GetCurrentWrapperForContents(
        const TabContents* contents);

    TabContentsWrapperDelegate* delegate() const { return delegate_; }
    void set_delegate(TabContentsWrapperDelegate* d) { delegate_ = d; }

    TabContents* tab_contents() const { return tab_contents_.get(); }
    TabContentsView* view() const { return tab_contents()->view(); }

    // Overrides -----------------------------------------------------------------

    // TabContentsObserver overrides:
    virtual void RenderViewCreated(RenderViewHost* render_view_host);
    virtual void RenderViewGone();
    virtual void DidBecomeSelected();
    //virtual bool OnMessageReceived(const IPC::Message& message);
    virtual void TabContentsDestroyed(TabContents* tab);

private:
    // Internal helpers ----------------------------------------------------------

    // Message handlers.
    void OnSnapshot(const SkBitmap& bitmap);

    // Data for core operation ---------------------------------------------------

    // Delegate for notifying our owner about stuff. Not owned by us.
    TabContentsWrapperDelegate* delegate_;

    // TabContents (MUST BE LAST) ------------------------------------------------

    // If true, we're running the destructor.
    bool in_destructor_;

    // The supporting objects need to outlive the TabContents dtor (as they may
    // be called upon during its execution). As a result, this must come last
    // in the list.
    scoped_ptr<TabContents> tab_contents_;

    DISALLOW_COPY_AND_ASSIGN(TabContentsWrapper);
};

#endif //__tab_contents_wrapper_h__