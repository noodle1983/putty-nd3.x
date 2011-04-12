
#include "client_view.h"

#include "base/logging.h"

#include "../accessibility/accessible_view_state.h"
#include "../window/window.h"
#include "../window/window_delegate.h"

namespace view
{

    ///////////////////////////////////////////////////////////////////////////////
    // ClientView, public:

    ClientView::ClientView(Window* window, View* contents_view)
        : window_(window), contents_view_(contents_view) {}

    DialogClientView* ClientView::AsDialogClientView()
    {
        return NULL;
    }

    bool ClientView::CanClose()
    {
        return true;
    }

    void ClientView::WindowClosing() {}

    int ClientView::NonClientHitTest(const gfx::Point& point)
    {
        return bounds().Contains(point) ? HTCLIENT : HTNOWHERE;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // ClientView, View overrides:

    gfx::Size ClientView::GetPreferredSize()
    {
        // |contents_view_| is allowed to be NULL up until the point where this view
        // is attached to a Container.
        if(contents_view_)
        {
            return contents_view_->GetPreferredSize();
        }
        return gfx::Size();
    }

    void ClientView::Layout()
    {
        // |contents_view_| is allowed to be NULL up until the point where this view
        // is attached to a Container.
        if(contents_view_)
        {
            contents_view_->SetBounds(0, 0, width(), height());
        }
    }

    void ClientView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && child==this)
        {
            DCHECK(GetWidget());
            DCHECK(contents_view_); // |contents_view_| must be valid now!
            // Insert |contents_view_| at index 0 so it is first in the focus chain.
            // (the OK/Cancel buttons are inserted before contents_view_)
            AddChildViewAt(contents_view_, 0);
        }
    }

    void ClientView::OnBoundsChanged(const gfx::Rect& previous_bounds)
    {
        // Overridden to do nothing. The NonClientView manually calls Layout on the
        // ClientView when it is itself laid out, see comment in
        // NonClientView::Layout.
    }

    void ClientView::GetAccessibleState(AccessibleViewState* state)
    {
        state->role = AccessibilityTypes::ROLE_CLIENT;
    }

} //namespace view