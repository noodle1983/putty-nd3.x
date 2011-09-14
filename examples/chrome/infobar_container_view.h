
#ifndef __infobar_container_view_h__
#define __infobar_container_view_h__

#pragma once

#include "infobar_container.h"
#include "accessible_pane_view.h"

// The views-specific implementation of InfoBarContainer.
class InfoBarContainerView : public AccessiblePaneView,
    public InfoBarContainer
{
public:
    explicit InfoBarContainerView(Delegate* delegate);
    virtual ~InfoBarContainerView();

private:
    // AccessiblePaneView:
    virtual gfx::Size GetPreferredSize() OVERRIDE;
    virtual void Layout() OVERRIDE;
    virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

    // InfobarContainer:
    virtual void PlatformSpecificAddInfoBar(InfoBar* infobar,
        size_t position) OVERRIDE;
    virtual void PlatformSpecificRemoveInfoBar(InfoBar* infobar) OVERRIDE;

    DISALLOW_COPY_AND_ASSIGN(InfoBarContainerView);
};

#endif //__infobar_container_view_h__