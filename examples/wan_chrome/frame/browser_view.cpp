
#include "browser_view.h"

#include "base/logging.h"

#include "SkBitmap.h"

#include "view/accessibility/accessible_view_state.h"
#include "view/base/view_prop.h"
#include "view/l10n/l10n_util.h"
#include "view/widget/native_widget.h"

#include "../../wanui_res/resource.h"

#include "browser_view_layout.h"

// The name of a key to store on the window handle so that other code can
// locate this object using just the handle.
static const char* const kBrowserViewKey = "__BROWSER_VIEW__";

const char BrowserView::kViewClassName[] = "browser/view/BrowserView";

///////////////////////////////////////////////////////////////////////////////
// BrowserView, public:

BrowserView::BrowserView()
: view::ClientView(NULL, NULL),
frame_(NULL),
contents_(NULL),
initialized_(false)
{
}

BrowserView::~BrowserView()
{
    // Child views maintain PrefMember attributes that point to
    // OffTheRecordProfile's PrefService which gets deleted by ~Browser.
    RemoveAllChildViews(true);
}

// static
BrowserView* BrowserView::GetBrowserViewForNativeWindow(HWND window)
{
    if(IsWindow(window))
    {
        return reinterpret_cast<BrowserView*>(
            view::ViewProp::GetValue(window, kBrowserViewKey));
    }
    return NULL;
}

gfx::Rect BrowserView::GetClientAreaBounds() const
{
    gfx::Rect container_bounds = contents_->bounds();
    gfx::Point container_origin = container_bounds.origin();
    ConvertPointToView(this, parent(), &container_origin);
    container_bounds.set_origin(container_origin);
    return container_bounds;
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
    return true;
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
    return contents_;
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

int BrowserView::NonClientHitTest(const gfx::Point& point)
{
    return GetBrowserViewLayout()->NonClientHitTest(point);
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
    // Stow a pointer to this object onto the window handle so that we can get at
    // it later when all we have is a native view.
    GetWidget()->native_widget()->SetNativeWindowProperty(kBrowserViewKey, this);

    contents_ = new view::View;
    contents_->set_background(view::Background::CreateStandardPanelBackground());

    set_contents_view(contents_);
}

BrowserViewLayout* BrowserView::GetBrowserViewLayout() const
{
    return static_cast<BrowserViewLayout*>(GetLayoutManager());
}