
#include "browser_root_view.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/dragdrop/drag_drop_types.h"
#include "ui_base/dragdrop/os_exchange_data.h"
#include "ui_base/l10n/l10n_util.h"

#include "../wanui_res/resource.h"

#include "browser_frame.h"
#include "browser_view.h"

// static
const char BrowserRootView::kViewClassName[] = "browser/BrowserRootView";

BrowserRootView::BrowserRootView(BrowserView* browser_view,
                                 view::Widget* widget)
                                 : view::internal::RootView(widget),
                                 browser_view_(browser_view),
                                 forwarding_to_tab_strip_(false) {}

bool BrowserRootView::GetDropFormats(int* formats,
                                     std::set<ui::OSExchangeData::CustomFormat>* custom_formats)
{
    if(tabstrip() && tabstrip()->IsVisible())
    {
        *formats = /*ui::OSExchangeData::URL | */ui::OSExchangeData::STRING;
        return true;
    }
    return false;
}

bool BrowserRootView::AreDropTypesRequired()
{
    return true;
}

bool BrowserRootView::CanDrop(const ui::OSExchangeData& data)
{
    if(!tabstrip() || !tabstrip()->IsVisible())
    {
        return false;
    }

    // If there is a URL, we'll allow the drop.
    //if(data.HasURL())
    //{
    //    return true;
    //}

    // If there isn't a URL, see if we can 'paste and go'.
    return GetPasteAndGoURL(data, NULL);
}

void BrowserRootView::OnDragEntered(const view::DropTargetEvent& event)
{
    if(ShouldForwardToTabStrip(event))
    {
        forwarding_to_tab_strip_ = true;
        scoped_ptr<view::DropTargetEvent> mapped_event(
            MapEventToTabStrip(event, event.data()));
        //tabstrip()->OnDragEntered(*mapped_event.get());
    }
}

int BrowserRootView::OnDragUpdated(const view::DropTargetEvent& event)
{
    if(ShouldForwardToTabStrip(event))
    {
        scoped_ptr<view::DropTargetEvent> mapped_event(
            MapEventToTabStrip(event, event.data()));
        if(!forwarding_to_tab_strip_)
        {
            //tabstrip()->OnDragEntered(*mapped_event.get());
            forwarding_to_tab_strip_ = true;
        }
        //return tabstrip()->OnDragUpdated(*mapped_event.get());
    }
    else if(forwarding_to_tab_strip_)
    {
        forwarding_to_tab_strip_ = false;
        //tabstrip()->OnDragExited();
    }
    return ui::DragDropTypes::DRAG_NONE;
}

void BrowserRootView::OnDragExited()
{
    if(forwarding_to_tab_strip_)
    {
        forwarding_to_tab_strip_ = false;
        //tabstrip()->OnDragExited();
    }
}

int BrowserRootView::OnPerformDrop(const view::DropTargetEvent& event)
{
    //if(!forwarding_to_tab_strip_)
    {
        return ui::DragDropTypes::DRAG_NONE;
    }

    // Extract the URL and create a new ui::OSExchangeData containing the URL. We
    // do this as the TabStrip doesn't know about the autocomplete edit and needs
    // to know about it to handle 'paste and go'.
    //Url url;
    //string16 title;
    //ui::OSExchangeData mapped_data;
    //if(!event.data().GetURLAndTitle(&url, &title) || !url.is_valid())
    //{
    //    // The url isn't valid. Use the paste and go url.
    //    if(GetPasteAndGoURL(event.data(), &url))
    //    {
    //        mapped_data.SetURL(url, string16());
    //    }
    //    // else case: couldn't extract a url or 'paste and go' url. This ends up
    //    // passing through an ui::OSExchangeData with nothing in it. We need to do
    //    // this so that the tab strip cleans up properly.
    //}
    //else
    //{
    //    mapped_data.SetURL(url, string16());
    //}
    //forwarding_to_tab_strip_ = false;
    //scoped_ptr<view::DropTargetEvent> mapped_event(
    //    MapEventToTabStrip(event, mapped_data));
    //return tabstrip()->OnPerformDrop(*mapped_event);
}

void BrowserRootView::GetAccessibleState(ui::AccessibleViewState* state)
{
    view::internal::RootView::GetAccessibleState(state);
    state->name = ui::GetStringUTF16(IDS_PRODUCT_NAME);
}

std::string BrowserRootView::GetClassName() const
{
    return kViewClassName;
}

bool BrowserRootView::ShouldForwardToTabStrip(
    const view::DropTargetEvent& event)
{
    if(!tabstrip()->IsVisible())
    {
        return false;
    }

    // Allow the drop as long as the mouse is over the tabstrip or vertically
    // before it.
    gfx::Point tab_loc_in_host;
    ConvertPointToView(tabstrip(), this, &tab_loc_in_host);
    return event.y() < tab_loc_in_host.y() + tabstrip()->height();
}

view::DropTargetEvent* BrowserRootView::MapEventToTabStrip(
    const view::DropTargetEvent& event,
    const ui::OSExchangeData& data)
{
    gfx::Point tab_strip_loc(event.location());
    ConvertPointToView(this, tabstrip(), &tab_strip_loc);
    return new view::DropTargetEvent(data, tab_strip_loc.x(),
        tab_strip_loc.y(), event.source_operations());
}

AbstractTabStripView* BrowserRootView::tabstrip() const
{
    return browser_view_->tabstrip();
}

bool BrowserRootView::GetPasteAndGoURL(const ui::OSExchangeData& data,
                                       Url* url)
{
    if(!data.HasString())
    {
        return false;
    }

    string16 text;
    if(!data.GetString(&text) || text.empty())
    {
        return false;
    }

    //AutocompleteMatch match;
    //browser_view_->browser()->profile()->GetAutocompleteClassifier()->Classify(
    //    text, string16(), false, false, &match, NULL);
    //if(!match.destination_url.is_valid())
    //{
    //    return false;
    //}

    //if(url)
    //{
    //    *url = match.destination_url;
    //}
    return true;
}