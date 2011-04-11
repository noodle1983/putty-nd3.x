
#include "accessible_pane_view.h"

#include "message_framework/message_loop.h"

#include "view_framework/accessibility/accessible_view_state.h"
#include "view_framework/controls/native/native_view_host.h"
#include "view_framework/focus/focus_search.h"
#include "view_framework/focus/view_storage.h"

AccessiblePaneView::AccessiblePaneView()
: pane_has_focus_(false),
method_factory_(this),
focus_manager_(NULL),
home_key_(view::VKEY_HOME, false, false, false),
end_key_(view::VKEY_END, false, false, false),
escape_key_(view::VKEY_ESCAPE, false, false, false),
left_key_(view::VKEY_LEFT, false, false, false),
right_key_(view::VKEY_RIGHT, false, false, false),
last_focused_view_storage_id_(-1)
{
    focus_search_.reset(new view::FocusSearch(this, true, true));
}

AccessiblePaneView::~AccessiblePaneView()
{
    if(pane_has_focus_)
    {
        focus_manager_->RemoveFocusChangeListener(this);
    }
}

bool AccessiblePaneView::SetPaneFocus(int view_storage_id,
                                      view::View* initial_focus)
{
    if(!IsVisible())
    {
        return false;
    }

    // Save the storage id to the last focused view. This would be used to request
    // focus to the view when the traversal is ended.
    last_focused_view_storage_id_ = view_storage_id;

    if(!focus_manager_)
    {
        focus_manager_ = GetFocusManager();
    }

    // Use the provided initial focus if it's visible and enabled, otherwise
    // use the first focusable child.
    if(!initial_focus || !Contains(initial_focus) ||
        !initial_focus->IsVisible() || !initial_focus->IsEnabled())
    {
        initial_focus = GetFirstFocusableChild();
    }

    // Return false if there are no focusable children.
    if(!initial_focus)
    {
        return false;
    }

    // Set focus to the initial view. If it's a location bar, use a special
    // method that tells it to select all, also.
    if(initial_focus->GetClassName() == LocationBarView::kViewClassName)
    {
        static_cast<LocationBarView*>(initial_focus)->FocusLocation(true);
    }
    else
    {
        focus_manager_->SetFocusedView(initial_focus);
    }

    // If we already have pane focus, we're done.
    if(pane_has_focus_)
    {
        return true;
    }

    // Otherwise, set accelerators and start listening for focus change events.
    pane_has_focus_ = true;
    focus_manager_->RegisterAccelerator(home_key_, this);
    focus_manager_->RegisterAccelerator(end_key_, this);
    focus_manager_->RegisterAccelerator(escape_key_, this);
    focus_manager_->RegisterAccelerator(left_key_, this);
    focus_manager_->RegisterAccelerator(right_key_, this);
    focus_manager_->AddFocusChangeListener(this);

    return true;
}

bool AccessiblePaneView::SetPaneFocusAndFocusDefault(int view_storage_id)
{
    return SetPaneFocus(view_storage_id, GetDefaultFocusableChild());
}

view::View* AccessiblePaneView::GetDefaultFocusableChild()
{
    return NULL;
}

void AccessiblePaneView::RemovePaneFocus()
{
    focus_manager_->RemoveFocusChangeListener(this);
    pane_has_focus_ = false;

    focus_manager_->UnregisterAccelerator(home_key_, this);
    focus_manager_->UnregisterAccelerator(end_key_, this);
    focus_manager_->UnregisterAccelerator(escape_key_, this);
    focus_manager_->UnregisterAccelerator(left_key_, this);
    focus_manager_->UnregisterAccelerator(right_key_, this);
}

void AccessiblePaneView::LocationBarSelectAll()
{
    view::View* focused_view = GetFocusManager()->GetFocusedView();
    if(focused_view &&
        focused_view->GetClassName()==LocationBarView::kViewClassName)
    {
        static_cast<LocationBarView*>(focused_view)->SelectAll();
    }
}

void AccessiblePaneView::RestoreLastFocusedView()
{
    view::ViewStorage* view_storage = view::ViewStorage::GetInstance();
    view::View* last_focused_view =
        view_storage->RetrieveView(last_focused_view_storage_id_);
    if(last_focused_view)
    {
        focus_manager_->SetFocusedViewWithReason(
            last_focused_view, view::FocusManager::kReasonFocusRestore);
    }
    else
    {
        // Focus the location bar
        view::View* view = GetAncestorWithClassName(BrowserView::kViewClassName);
        if(view)
        {
            BrowserView* browser_view = static_cast<BrowserView*>(view);
            browser_view->SetFocusToLocationBar(false);
        }
    }
}

view::View* AccessiblePaneView::GetFirstFocusableChild()
{
    FocusTraversable* dummy_focus_traversable;
    view::View* dummy_focus_traversable_view;
    return focus_search_->FindNextFocusableView(
        NULL, false, view::FocusSearch::DOWN, false,
        &dummy_focus_traversable, &dummy_focus_traversable_view);
}

view::View* AccessiblePaneView::GetLastFocusableChild()
{
    FocusTraversable* dummy_focus_traversable;
    view::View* dummy_focus_traversable_view;
    return focus_search_->FindNextFocusableView(
        this, true, view::FocusSearch::DOWN, false,
        &dummy_focus_traversable, &dummy_focus_traversable_view);
}

////////////////////////////////////////////////////////////////////////////////
// View overrides:

view::FocusTraversable* AccessiblePaneView::GetPaneFocusTraversable()
{
    if(pane_has_focus_)
    {
        return this;
    }
    else
    {
        return NULL;
    }
}

bool AccessiblePaneView::AcceleratorPressed(
    const view::Accelerator& accelerator)
{
    // Special case: don't handle any accelerators for the location bar,
    // so that it behaves exactly the same whether you focus it with Ctrl+L
    // or F6 or Alt+D or Alt+Shift+T.
    view::View* focused_view = focus_manager_->GetFocusedView();
    if((focused_view->GetClassName()==LocationBarView::kViewClassName ||
        focused_view->GetClassName()==view::NativeViewHost::kViewClassName))
    {
        return false;
    }

    switch(accelerator.GetKeyCode())
    {
    case view::VKEY_ESCAPE:
        RemovePaneFocus();
        RestoreLastFocusedView();
        return true;
    case view::VKEY_LEFT:
        focus_manager_->AdvanceFocus(true);
        return true;
    case view::VKEY_RIGHT:
        focus_manager_->AdvanceFocus(false);
        return true;
    case view::VKEY_HOME:
        focus_manager_->SetFocusedViewWithReason(
            GetFirstFocusableChild(), view::FocusManager::kReasonFocusTraversal);
        return true;
    case view::VKEY_END:
        focus_manager_->SetFocusedViewWithReason(
            GetLastFocusableChild(), view::FocusManager::kReasonFocusTraversal);
        return true;
    default:
        return false;
    }
}

void AccessiblePaneView::SetVisible(bool flag)
{
    if(IsVisible() && !flag && pane_has_focus_)
    {
        RemovePaneFocus();
        RestoreLastFocusedView();
    }
    View::SetVisible(flag);
}

void AccessiblePaneView::GetAccessibleState(AccessibleViewState* state)
{
    state->role = AccessibilityTypes::ROLE_PANE;
}

////////////////////////////////////////////////////////////////////////////////
// FocusChangeListener overrides:

void AccessiblePaneView::FocusWillChange(view::View* focused_before,
                                         view::View* focused_now)
{
    if(!focused_now)
    {
        return;
    }

    view::FocusManager::FocusChangeReason reason =
        focus_manager_->focus_change_reason();

    if(focused_now->GetClassName()==LocationBarView::kViewClassName &&
        reason==view::FocusManager::kReasonFocusTraversal)
    {
        // Tabbing to the location bar should select all. Defer so that it happens
        // after the focus.
        MessageLoop::current()->PostTask(method_factory_.NewRunnableMethod(
            &AccessiblePaneView::LocationBarSelectAll));
    }

    if(!Contains(focused_now) ||
        reason==view::FocusManager::kReasonDirectFocusChange)
    {
        // We should remove pane focus (i.e. make most of the controls
        // not focusable again) either because the focus is leaving the pane,
        // or because the focus changed within the pane due to the user
        // directly focusing to a specific view (e.g., clicking on it).
        //
        // Defer rather than calling RemovePaneFocus right away, because we can't
        // remove |this| as a focus change listener while FocusManager is in the
        // middle of iterating over the list of listeners.
        MessageLoop::current()->PostTask(method_factory_.NewRunnableMethod(
            &AccessiblePaneView::RemovePaneFocus));
    }
}

////////////////////////////////////////////////////////////////////////////////
// FocusTraversable overrides:

view::FocusSearch* AccessiblePaneView::GetFocusSearch()
{
    DCHECK(pane_has_focus_);
    return focus_search_.get();
}

view::FocusTraversable* AccessiblePaneView::GetFocusTraversableParent()
{
    DCHECK(pane_has_focus_);
    return NULL;
}

view::View* AccessiblePaneView::GetFocusTraversableParentView()
{
    DCHECK(pane_has_focus_);
    return NULL;
}