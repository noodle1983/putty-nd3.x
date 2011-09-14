
#include "location_icon_view.h"

#include "ui_base/l10n/l10n_util.h"

#include "../wanui_res/resource.h"

LocationIconView::LocationIconView(LocationBarView* location_bar)
{
    SetTooltipText(ui::GetStringUTF16(IDS_TOOLTIP_LOCATION_ICON));
}

LocationIconView::~LocationIconView() {}

bool LocationIconView::OnMousePressed(const view::MouseEvent& event)
{
    // We want to show the dialog on mouse release; that is the standard behavior
    // for buttons.
    return true;
}

void LocationIconView::OnMouseReleased(const view::MouseEvent& event)
{
    //click_handler_.OnMouseReleased(event);
}

void LocationIconView::ShowTooltip(bool show)
{
    if(show)
    {
        SetTooltipText(ui::GetStringUTF16(IDS_TOOLTIP_LOCATION_ICON));
    }
    else
    {
        SetTooltipText(L"");
    }
}