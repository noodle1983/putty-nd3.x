
#ifndef __tab_handler_h__
#define __tab_handler_h__

#pragma once

class Browser;
class TabStripModel;

class TabHandlerDelegate
{
public:
    // TODO(beng): remove once decoupling with Browser is complete.
    virtual Browser* AsBrowser() = 0;
};

// An interface implemented by an object that can perform tab related
// functionality for a Browser. This functionality includes mapping individual
// TabContentses into indices for an index-based tab organization scheme for
// example.
class TabHandler
{
public:
    virtual ~TabHandler() {}

    // Creates a TabHandler implementation and returns it, transferring ownership
    // to the caller.
    static TabHandler* CreateTabHandler(TabHandlerDelegate* delegate);

    // TODO(beng): remove once decoupling with Browser is complete.
    virtual TabStripModel* GetTabStripModel() const = 0;
};

#endif //__tab_handler_h__