
#include "tab_contents_container_views.h"

#include "ui_base/accessibility/accessible_view_state.h"

#include "view/layout/fill_layout.h"

#include "view_ids.h"

// static
const char TabContentsContainer::kViewClassName[] =
    "browser/TabContentsContainer";

// Some of this class is implemented in tab_contents_container.cc, where
// the implementation doesn't vary between a pure views approach and a
// native view host approach. See the header file for details.

////////////////////////////////////////////////////////////////////////////////
// TabContentsContainer, public:

TabContentsContainer::TabContentsContainer() : tab_contents_(NULL)
{
    set_id(VIEW_ID_TAB_CONTAINER);
}

void TabContentsContainer::SetReservedContentsRect(
    const gfx::Rect& reserved_rect)
{
    cached_reserved_rect_ = reserved_rect;
    // TODO(anicolao): find out what this is supposed to be used for and ensure
    // it's OK for touch.
}

void TabContentsContainer::ChangeTabContents(TabContents* contents)
{
    //if(tab_contents_)
    //{
    //    view::View *v = static_cast<TabContentsViewTouch*>(tab_contents_->view());
    //    RemoveChildView(v);
    //    tab_contents_->WasHidden();
    //    RemoveObservers();
    //}
    //tab_contents_ = contents;
    //// When detaching the last tab of the browser ChangeTabContents is invoked
    //// with NULL. Don't attempt to do anything in that case.
    //if(tab_contents_)
    //{
    //    view::View *v = static_cast<TabContentsViewTouch*>(contents->view());
    //    AddChildView(v);
    //    SetLayoutManager(new view::FillLayout());
    //    Layout();
    //    AddObservers();
    //}
}

std::string TabContentsContainer::GetClassName() const
{
  return kViewClassName;
}

void TabContentsContainer::TabContentsFocused(TabContents* tab_contents)
{
}

void TabContentsContainer::SetFastResize(bool fast_resize)
{
}


////////////////////////////////////////////////////////////////////////////////
// TabContentsContainer, public:

TabContentsContainer::~TabContentsContainer()
{
    if(tab_contents_)
    {
        RemoveObservers();
    }
}

////////////////////////////////////////////////////////////////////////////////
// TabContentsContainer, View overrides:

void TabContentsContainer::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_WINDOW;
}

////////////////////////////////////////////////////////////////////////////////
// TabContentsContainer, private:

void TabContentsContainer::AddObservers()
{
}

void TabContentsContainer::RemoveObservers()
{
}

void TabContentsContainer::TabContentsDestroyed(TabContents* contents)
{
    // Sometimes, a TabContents is destroyed before we know about it. This allows
    // us to clean up our state in case this happens.
    DCHECK(contents == tab_contents_);
    ChangeTabContents(NULL);
}

void TabContentsContainer::RenderWidgetHostViewChanged(
    RenderWidgetHostView* new_view)
{
    if(new_view)
    {
        //new_view->set_reserved_contents_rect(cached_reserved_rect_);
    }
}