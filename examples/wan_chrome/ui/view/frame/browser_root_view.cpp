
#include "browser_root_view.h"

#include "view/accessibility/accessible_view_state.h"
#include "view/dragdrop/drag_drop_types.h"
#include "view/l10n/l10n_util.h"

#include "../../../../wanui_res/resource.h"

BrowserRootView::BrowserRootView(BrowserView* browser_view,
                                 view::Widget* widget)
                                 : view::RootView(widget),
                                 browser_view_(browser_view),
                                 forwarding_to_tab_strip_(false) {}

bool BrowserRootView::GetDropFormats(int* formats,
                                     std::set<OSExchangeData::CustomFormat>* custom_formats)
{
    return false;
}

bool BrowserRootView::AreDropTypesRequired()
{
    return true;
}

bool BrowserRootView::CanDrop(const OSExchangeData& data)
{
    return false;
}

void BrowserRootView::OnDragEntered(const view::DropTargetEvent& event)
{
}

int BrowserRootView::OnDragUpdated(const view::DropTargetEvent& event)
{
    return DragDropTypes::DRAG_NONE;
}

void BrowserRootView::OnDragExited()
{
    if(forwarding_to_tab_strip_)
    {
        forwarding_to_tab_strip_ = false;
    }
}

int BrowserRootView::OnPerformDrop(const view::DropTargetEvent& event)
{
    if(!forwarding_to_tab_strip_)
    {
        return DragDropTypes::DRAG_NONE;
    }
}

void BrowserRootView::GetAccessibleState(AccessibleViewState* state)
{
    RootView::GetAccessibleState(state);
    state->name = view::GetStringUTF16(IDS_PRODUCT_NAME);
}