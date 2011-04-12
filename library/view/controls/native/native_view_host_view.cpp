
#include "native_view_host_view.h"

#include "base/logging.h"

#include "gfx/canvas.h"

#include "../../focus/focus_manager.h"
#include "../../view/root_view.h"
#include "../../widget/widget.h"
#include "native_view_host.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // NativeViewHostView, public:

    NativeViewHostView::NativeViewHostView(NativeViewHost* host)
        : host_(host), installed_clip_(false) {}

    NativeViewHostView::~NativeViewHostView()
    {
        NOTIMPLEMENTED();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeViewHostView, NativeViewHostWrapper implementation:
    void NativeViewHostView::NativeViewAttached()
    {
        host_->AddChildView(host_->view_view());
        host_->Layout();
    }

    void NativeViewHostView::NativeViewDetaching(bool destroyed)
    {
        host_->RemoveChildView(host_->view_view());
    }

    void NativeViewHostView::AddedToWidget()
    {
        // nothing to do
    }

    void NativeViewHostView::RemovedFromWidget()
    {
        // nothing to do
    }

    void NativeViewHostView::InstallClip(int x, int y, int w, int h)
    {
        NOTIMPLEMENTED();
    }

    bool NativeViewHostView::HasInstalledClip()
    {
        return installed_clip_;
    }

    void NativeViewHostView::UninstallClip()
    {
        installed_clip_ = false;
    }

    void NativeViewHostView::ShowWidget(int x, int y, int w, int h)
    {
        // x, y are in the coordinate system of the root view, but we're
        // already properly positioned by virtue of being an actual views
        // child of the NativeHostView, so disregard the origin.
        host_->view_view()->SetBounds(0, 0, w, h);
        host_->view_view()->SetVisible(true);
    }

    void NativeViewHostView::HideWidget()
    {
        host_->view_view()->SetVisible(false);
    }

    void NativeViewHostView::SetFocus()
    {
        host_->view_view()->RequestFocus();
    }

} //namespace view