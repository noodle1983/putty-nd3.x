
#include "tab_strip_factory.h"

#include "browser_tab_strip_controller.h"
#include "tab_strip.h"

// This default implementation of CreateTabStrip creates a TabStrip or a
// SideTabStrip, depending on whether we are using vertical tabs.
AbstractTabStripView* CreateTabStrip(Browser* browser,
                                     view::View* parent,
                                     TabStripModel* model)
{
    BrowserTabStripController* tabstrip_controller =
        new BrowserTabStripController(browser, model);
    // Ownership of this controller is given to a specific tabstrip when we
    // construct it below.

    BaseTabStrip* tabstrip = new TabStrip(tabstrip_controller);
    parent->AddChildView(tabstrip);
    tabstrip_controller->InitFromModel(tabstrip);
    return tabstrip;
}