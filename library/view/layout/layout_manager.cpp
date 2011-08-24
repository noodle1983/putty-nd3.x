
#include "layout_manager.h"

#include "ui_gfx/size.h"

namespace view
{

    LayoutManager::~LayoutManager() {}

    void LayoutManager::Installed(View* host) {}

    void LayoutManager::Uninstalled(View* host) {}

    int LayoutManager::GetPreferredHeightForWidth(View* host, int width)
    {
        return GetPreferredSize(host).height();
    }

    void LayoutManager::ViewAdded(View* host, View* view) {}

    void LayoutManager::ViewRemoved(View* host, View* view) {}

} //namespace view