
// The LocationBar class is a virtual interface, defining access to the
// window's location bar component.  This class exists so that cross-platform
// components like the browser command system can talk to the platform
// specific implementations of the location bar control.

#ifndef __location_bar_h__
#define __location_bar_h__

#pragma once

#include "base/string16.h"

#include "window_open_disposition.h"

class OmniboxView;
class TabContents;
class TabContentsWrapper;

class LocationBar
{
public:
    // Sets the suggested text to show in the omnibox. This is shown in addition
    // to the current text of the omnibox.
    virtual void SetSuggestedText(const string16& text) = 0;

    // Returns the string of text entered in the location bar.
    virtual string16 GetInputString() const = 0;

    // Returns the WindowOpenDisposition that should be used to determine where
    // to open a URL entered in the location bar.
    virtual WindowOpenDisposition GetWindowOpenDisposition() const = 0;

    // Accepts the current string of text entered in the location bar.
    virtual void AcceptInput() = 0;

    // Focuses the location bar.  Optionally also selects its contents.
    virtual void FocusLocation(bool select_all) = 0;

    // Clears the location bar, inserts an annoying little "?" turd and sets
    // focus to it.
    virtual void FocusSearch() = 0;

    // Updates the state of the images showing the content settings status.
    virtual void UpdateContentSettingsIcons() = 0;

    // Updates the state of the page actions.
    virtual void UpdatePageActions() = 0;

    // Called when the page-action data needs to be refreshed, e.g. when an
    // extension is unloaded or crashes.
    virtual void InvalidatePageActions() = 0;

    // Saves the state of the location bar to the specified TabContents, so that
    // it can be restored later. (Done when switching tabs).
    virtual void SaveStateToContents(TabContents* contents) = 0;

    // Reverts the location bar.  The bar's permanent text will be shown.
    virtual void Revert() = 0;

    // Returns a pointer to the text entry view.
    virtual const OmniboxView* location_entry() const = 0;
    virtual OmniboxView* location_entry() = 0;

protected:
    virtual ~LocationBar() {}
};

#endif //__location_bar_h__