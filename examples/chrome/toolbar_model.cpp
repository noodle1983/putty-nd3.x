
#include "toolbar_model.h"

#include "browser.h"

ToolbarModel::ToolbarModel(Browser* browser)
: browser_(browser), input_in_progress_(false) {}

ToolbarModel::~ToolbarModel() {}

// ToolbarModel Implementation.
string16 ToolbarModel::GetText() const
{
    return string16();
    //Url url(chrome::kAboutBlankURL);
    //std::string languages; // Empty if we don't have a |navigation_controller|.

    //NavigationController* navigation_controller = GetNavigationController();
    //if(navigation_controller)
    //{
    //    Profile* profile =
    //        Profile::FromBrowserContext(navigation_controller->browser_context());
    //    languages = profile->GetPrefs()->GetString(prefs::kAcceptLanguages);
    //    NavigationEntry* entry = navigation_controller->GetVisibleEntry();
    //    if(!navigation_controller->tab_contents()->ShouldDisplayURL())
    //    {
    //        // Explicitly hide the URL for this tab.
    //        url = Url();
    //    }
    //    else if(entry)
    //    {
    //        url = entry->virtual_url();
    //    }
    //}
    //if(url.spec().length() > content::kMaxURLDisplayChars)
    //{
    //    url = url.IsStandard() ? url.GetOrigin() : Url(url.scheme() + ":");
    //}
    //// Note that we can't unescape spaces here, because if the user copies this
    //// and pastes it into another program, that program may think the URL ends at
    //// the space.
    //return AutocompleteInput::FormattedStringWithEquivalentMeaning(
    //    url, net::FormatUrl(url, languages, net::kFormatUrlOmitAll,
    //    UnescapeRule::NORMAL, NULL, NULL, NULL));
}

int ToolbarModel::GetIcon() const
{
    return 0;
    //static int icon_ids[NUM_SECURITY_LEVELS] =
    //{
    //    IDR_OMNIBOX_HTTP,
    //    IDR_OMNIBOX_HTTPS_VALID,
    //    IDR_OMNIBOX_HTTPS_VALID,
    //    IDR_OMNIBOX_HTTPS_WARNING,
    //    IDR_OMNIBOX_HTTPS_INVALID,
    //};
    //DCHECK(arraysize(icon_ids) == NUM_SECURITY_LEVELS);
    //return icon_ids[GetSecurityLevel()];
}

NavigationController* ToolbarModel::GetNavigationController() const
{
    // This |current_tab| can be NULL during the initialization of the
    // toolbar during window creation (i.e. before any tabs have been added
    // to the window).
    //TabContents* current_tab = browser_->GetSelectedTabContents();
    //return current_tab ? &current_tab->controller() : NULL;
    return NULL;
}