
#include "status_icon.h"

#include "ui_base/models/menu_model.h"

StatusIcon::StatusIcon() {}

StatusIcon::~StatusIcon() {}

void StatusIcon::AddObserver(Observer* observer)
{
    observers_.AddObserver(observer);
}

void StatusIcon::RemoveObserver(Observer* observer)
{
    observers_.RemoveObserver(observer);
}

bool StatusIcon::HasObservers()
{
    return observers_.size() > 0;
}

void StatusIcon::DispatchClickEvent()
{
    FOR_EACH_OBSERVER(Observer, observers_, OnClicked());
}

void StatusIcon::SetContextMenu(ui::MenuModel* menu)
{
    context_menu_contents_.reset(menu);
    UpdatePlatformContextMenu(menu);
}