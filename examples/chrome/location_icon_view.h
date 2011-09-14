
#ifndef __location_icon_view_h__
#define __location_icon_view_h__

#pragma once

#include "view/controls/image_view.h"

namespace view
{
    class MouseEvent;
}

class LocationBarView;

// LocationIconView is used to display an icon to the left of the edit field.
// This shows the user's current action while editing, the page security
// status on https pages, or a globe for other URLs.
class LocationIconView : public view::ImageView
{
public:
    explicit LocationIconView(LocationBarView* location_bar);
    virtual ~LocationIconView();

    // Overridden from view::ImageView:
    virtual bool OnMousePressed(const view::MouseEvent& event) OVERRIDE;
    virtual void OnMouseReleased(const view::MouseEvent& event) OVERRIDE;

    // Whether we should show the tooltip for this icon or not.
    void ShowTooltip(bool show);

private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(LocationIconView);
};

#endif //__location_icon_view_h__