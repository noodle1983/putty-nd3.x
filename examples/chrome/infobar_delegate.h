
#ifndef __infobar_delegate_h__
#define __infobar_delegate_h__

#pragma once

#include "base/basic_types.h"
#include "base/string16.h"

#include "window_open_disposition.h"

namespace gfx
{
    class Image;
}

class InfoBar;
struct LoadCommittedDetails;
class TabContents;
class TabContentsWrapper;

// An interface implemented by objects wishing to control an InfoBar.
// Implementing this interface is not sufficient to use an InfoBar, since it
// does not map to a specific InfoBar type. Instead, you must implement either
// LinkInfoBarDelegate or ConfirmInfoBarDelegate, or override with your own
// delegate for your own InfoBar variety.
class InfoBarDelegate
{
public:
    // The type of the infobar. It controls its appearance, such as its background
    // color.
    enum Type
    {
        WARNING_TYPE,
        PAGE_ACTION_TYPE,
    };

    virtual ~InfoBarDelegate();

    // Called to create the InfoBar. Implementation of this method is
    // platform-specific.
    virtual InfoBar* CreateInfoBar(TabContentsWrapper* owner) = 0;

    // Returns true if the supplied |delegate| is equal to this one. Equality is
    // left to the implementation to define. This function is called by the
    // TabContentsWrapper when determining whether or not a delegate should be
    // added because a matching one already exists. If this function returns true,
    // the TabContentsWrapper will not add the new delegate because it considers
    // one to already be present.
    virtual bool EqualsDelegate(InfoBarDelegate* delegate) const;

    // Returns true if the InfoBar should be closed automatically after the page
    // is navigated. The default behavior is to return true if the
    // navigation is to a new page (not including reloads).
    virtual bool ShouldExpire(const LoadCommittedDetails& details) const;

    // Called when the user clicks on the close button to dismiss the infobar.
    virtual void InfoBarDismissed();

    // Called after the InfoBar is closed. Deletes |this|.
    // TODO(pkasting): Get rid of this and delete delegates directly.
    void InfoBarClosed();

    // Return the icon to be shown for this InfoBar. If the returned Image is
    // NULL, no icon is shown.
    virtual gfx::Image* GetIcon() const;

    // Returns the type of the infobar.  The type determines the appearance (such
    // as background color) of the infobar.
    virtual Type GetInfoBarType() const;

protected:
    // Provided to subclasses as a convenience to initialize the state of this
    // object. If |contents| is non-NULL, its active entry's unique ID will be
    // stored using StoreActiveEntryUniqueID automatically.
    explicit InfoBarDelegate(TabContents* contents);

    // Store the unique id for the active entry in the specified TabContents, to
    // be used later upon navigation to determine if this InfoBarDelegate should
    // be expired from |contents_|.
    void StoreActiveEntryUniqueID(TabContents* contents);

    // Returns true if the navigation is to a new URL or a reload occured.
    bool ShouldExpireInternal(const LoadCommittedDetails& details) const;

    // Removes ourself from |owner_| if we haven't already been removed.
    // TODO(pkasting): Move to InfoBar.
    void RemoveSelf();

private:
    // The unique id of the active NavigationEntry of the TabContents that we were
    // opened for. Used to help expire on navigations.
    int contents_unique_id_;

    // TODO(pkasting): Remove.
    TabContents* owner_;

    DISALLOW_COPY_AND_ASSIGN(InfoBarDelegate);
};

#endif //__infobar_delegate_h__