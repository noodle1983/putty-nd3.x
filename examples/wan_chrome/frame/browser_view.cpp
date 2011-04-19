
#include "browser_view.h"

#include "base/logging.h"

#include "SkBitmap.h"

#include "view/accessibility/accessible_view_state.h"
#include "view/l10n/l10n_util.h"

#include "../../wanui_res/resource.h"

#include "browser_view_layout.h"

// Returned from BrowserView::GetClassName.
const char BrowserView::kViewClassName[] = "browser/view/BrowserView";

///////////////////////////////////////////////////////////////////////////////
// BrowserView, public:

BrowserView::BrowserView()
: view::ClientView(NULL, NULL),
frame_(NULL),
contents_container_(NULL),
initialized_(false)
{
}

BrowserView::~BrowserView()
{
    // Child views maintain PrefMember attributes that point to
    // OffTheRecordProfile's PrefService which gets deleted by ~Browser.
    RemoveAllChildViews(true);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::WindowDelegate implementation:

bool BrowserView::CanResize() const
{
    return true;
}

bool BrowserView::CanMaximize() const
{
    return true;
}

bool BrowserView::CanActivate() const
{
    return true;
}

bool BrowserView::IsModal() const
{
    return false;
}

std::wstring BrowserView::GetWindowTitle() const
{
    return std::wstring(L"Wan-Chrome");
}

std::wstring BrowserView::GetAccessibleWindowTitle() const
{
    return GetWindowTitle();
}

view::View* BrowserView::GetInitiallyFocusedView()
{
    // We set the frame not focus on creation so this should never be called.
    NOTREACHED();
    return NULL;
}

bool BrowserView::ShouldShowWindowTitle() const
{
    return false;
}

SkBitmap BrowserView::GetWindowAppIcon()
{
    return GetWindowIcon();
}

SkBitmap BrowserView::GetWindowIcon()
{
    return SkBitmap();
}

bool BrowserView::ShouldShowWindowIcon() const
{
    return true;
}

view::View* BrowserView::GetContentsView()
{
    return contents_container_;
}

view::ClientView* BrowserView::CreateClientView(view::Window* window)
{
    set_window(window);
    return this;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::ClientView overrides:

bool BrowserView::CanClose()
{
    return true;
}

gfx::Size BrowserView::GetMinimumSize()
{
    return GetBrowserViewLayout()->GetMinimumSize();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::View overrides:

std::string BrowserView::GetClassName() const
{
    return kViewClassName;
}

void BrowserView::Layout()
{
    view::View::Layout();
}

void BrowserView::PaintChildren(gfx::Canvas* canvas)
{
    // Paint the |infobar_container_| last so that it may paint its
    // overlapping tabs.
    for(int i=0; i<child_count(); ++i)
    {
        View* child = GetChildViewAt(i);
        child->Paint(canvas);
    }
}

void BrowserView::ViewHierarchyChanged(bool is_add,
                                       view::View* parent,
                                       view::View* child)
{
    if(is_add && child==this && GetWidget() && !initialized_)
    {
        Init();
        initialized_ = true;
    }
}

void BrowserView::ChildPreferredSizeChanged(View* child)
{
    Layout();
}

void BrowserView::GetAccessibleState(AccessibleViewState* state)
{
    state->name = view::GetStringUTF16(IDS_PRODUCT_NAME);
    state->role = AccessibilityTypes::ROLE_CLIENT;
}

view::LayoutManager* BrowserView::CreateLayoutManager() const
{
    return new BrowserViewLayout;
}

void BrowserView::Init()
{
    SetLayoutManager(CreateLayoutManager());

    contents_container_ = new view::View;

    set_contents_view(contents_container_);
}

BrowserViewLayout* BrowserView::GetBrowserViewLayout() const
{
    return static_cast<BrowserViewLayout*>(GetLayoutManager());
}