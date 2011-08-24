
#ifndef __status_tray_h__
#define __status_tray_h__

#pragma once

#include <vector>

#include "base/basic_types.h"

class StatusIcon;

// Provides a cross-platform interface to the system's status tray, and exposes
// APIs to add/remove icons to the tray and attach context menus.
class StatusTray
{
public:
    // Static factory method that is implemented separately for each platform to
    // produce the appropriate platform-specific instance. Returns NULL if this
    // platform does not support status icons.
    static StatusTray* Create();

    virtual ~StatusTray();

    // Creates a new StatusIcon. The StatusTray retains ownership of the
    // StatusIcon. Returns NULL if the StatusIcon could not be created.
    StatusIcon* CreateStatusIcon();

    // Removes the current status icon associated with this identifier, if any.
    void RemoveStatusIcon(StatusIcon* icon);

protected:
    StatusTray();
    // Factory method for creating a status icon for this platform.
    virtual StatusIcon* CreatePlatformStatusIcon() = 0;

    // Removes all StatusIcons (used by derived classes to clean up in case they
    // track external state used by the StatusIcons).
    void RemoveAllIcons();

    typedef std::vector<StatusIcon*> StatusIconList;
    // Returns the list of active status icons so subclasses can operate on them.
    const StatusIconList& status_icons() { return status_icons_; }

private:
    // List containing all active StatusIcons.
    StatusIconList status_icons_;

    DISALLOW_COPY_AND_ASSIGN(StatusTray);
};

#endif //__status_tray_h__