
#ifndef __tab_strip_factory_h__
#define __tab_strip_factory_h__

#pragma once

class AbstractTabStripView;
class Browser;
class TabStripModel;

namespace view
{
    class View;
}

// Creates and returns a new tabstrip. The tabstrip should be added to |parent|.
AbstractTabStripView* CreateTabStrip(Browser* browser,
                                     view::View* parent,
                                     TabStripModel* model);

#endif //__tab_strip_factory_h__