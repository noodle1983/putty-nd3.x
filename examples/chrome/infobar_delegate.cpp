
#include "infobar_delegate.h"

#include "tab_contents_wrapper.h"

// InfoBarDelegate ------------------------------------------------------------

InfoBarDelegate::~InfoBarDelegate() {}

bool InfoBarDelegate::EqualsDelegate(InfoBarDelegate* delegate) const
{
    return false;
}

bool InfoBarDelegate::ShouldExpire(const LoadCommittedDetails& details) const
{
    //if(!details.is_navigation_to_different_page())
    //{
    //    return false;
    //}

    return ShouldExpireInternal(details);
}

void InfoBarDelegate::InfoBarDismissed() {}

void InfoBarDelegate::InfoBarClosed()
{
    delete this;
}

gfx::Image* InfoBarDelegate::GetIcon() const
{
    return NULL;
}

InfoBarDelegate::Type InfoBarDelegate::GetInfoBarType() const
{
    return WARNING_TYPE;
}

InfoBarDelegate::InfoBarDelegate(TabContents* contents)
: contents_unique_id_(0), owner_(contents)
{
    if(contents)
    {
        StoreActiveEntryUniqueID(contents);
    }
}

void InfoBarDelegate::StoreActiveEntryUniqueID(TabContents* contents)
{
    //NavigationEntry* active_entry = contents->controller().GetActiveEntry();
    //contents_unique_id_ = active_entry ? active_entry->unique_id() : 0;
}

bool InfoBarDelegate::ShouldExpireInternal(
    const LoadCommittedDetails& details) const
{
    //return (contents_unique_id_ != details.entry->unique_id()) ||
    //    (PageTransition::StripQualifier(details.entry->transition_type()) ==
    //    PageTransition::RELOAD);
    return false;
}

void InfoBarDelegate::RemoveSelf()
{
    if(owner_)
    {
        //TabContentsWrapper::GetCurrentWrapperForContents(owner_)->
        //    infobar_tab_helper()->RemoveInfoBar(this); // Clears |owner_|.
    }
}