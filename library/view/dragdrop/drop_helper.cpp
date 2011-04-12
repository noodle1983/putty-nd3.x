
#include "drop_helper.h"

#include "../view/root_view.h"
#include "drag_drop_types.h"

namespace view
{

    DropHelper::DropHelper(RootView* root_view)
        : root_view_(root_view),
        target_view_(NULL),
        deepest_view_(NULL) {}

    DropHelper::~DropHelper() {}

    void DropHelper::ResetTargetViewIfEquals(View* view)
    {
        if(target_view_ == view)
        {
            target_view_ = NULL;
        }
        if(deepest_view_ == view)
        {
            deepest_view_ = NULL;
        }
    }

    int DropHelper::OnDragOver(const OSExchangeData& data,
        const gfx::Point& root_view_location,
        int drag_operation)
    {
        View* view = CalculateTargetViewImpl(root_view_location, data,
            true, &deepest_view_);

        if(view != target_view_)
        {
            // 目标视图变化, 通知旧的目标视图退出, 新的目标视图进入.
            NotifyDragExit();
            target_view_ = view;
            NotifyDragEntered(data, root_view_location, drag_operation);
        }

        return NotifyDragOver(data, root_view_location, drag_operation);
    }

    void DropHelper::OnDragExit()
    {
        NotifyDragExit();
        deepest_view_ = target_view_ = NULL;
    }

    int DropHelper::OnDrop(const OSExchangeData& data,
        const gfx::Point& root_view_location,
        int drag_operation)
    {
        View* drop_view = target_view_;
        deepest_view_ = target_view_ = NULL;
        if(!drop_view)
        {
            return DragDropTypes::DRAG_NONE;
        }

        if(drag_operation == DragDropTypes::DRAG_NONE)
        {
            drop_view->OnDragExited();
            return DragDropTypes::DRAG_NONE;
        }

        gfx::Point view_location(root_view_location);
        View::ConvertPointToView(NULL, drop_view, &view_location);
        DropTargetEvent drop_event(data, view_location.x(), view_location.y(),
            drag_operation);
        return drop_view->OnPerformDrop(drop_event);
    }

    View* DropHelper::CalculateTargetView(
        const gfx::Point& root_view_location,
        const OSExchangeData& data,
        bool check_can_drop)
    {
        return CalculateTargetViewImpl(root_view_location, data,
            check_can_drop, NULL);
    }

    View* DropHelper::CalculateTargetViewImpl(
        const gfx::Point& root_view_location,
        const OSExchangeData& data,
        bool check_can_drop,
        View** deepest_view)
    {
        View* view = root_view_->GetEventHandlerForPoint(root_view_location);
        if(view == deepest_view_)
        {
            // 鼠标悬停的视图没有变化; 复用那个目标视图.
            return target_view_;
        }
        if(deepest_view)
        {
            *deepest_view = view;
        }
        while(view && view!=target_view_ &&
            (!view->IsEnabled() || !view->CanDrop(data)))
        {
            view = view->parent();
        }
        return view;
    }

    void DropHelper::NotifyDragEntered(const OSExchangeData& data,
        const gfx::Point& root_view_location,
        int drag_operation)
    {
        if(!target_view_)
        {
            return;
        }

        gfx::Point target_view_location(root_view_location);
        View::ConvertPointToView(root_view_, target_view_, &target_view_location);
        DropTargetEvent enter_event(data, target_view_location.x(),
            target_view_location.y(), drag_operation);
        target_view_->OnDragEntered(enter_event);
    }

    int DropHelper::NotifyDragOver(const OSExchangeData& data,
        const gfx::Point& root_view_location,
        int drag_operation)
    {
        if(!target_view_)
        {
            return DragDropTypes::DRAG_NONE;
        }

        gfx::Point target_view_location(root_view_location);
        View::ConvertPointToView(root_view_, target_view_, &target_view_location);
        DropTargetEvent enter_event(data, target_view_location.x(),
            target_view_location.y(), drag_operation);
        return target_view_->OnDragUpdated(enter_event);
    }

    void DropHelper::NotifyDragExit()
    {
        if(target_view_)
        {
            target_view_->OnDragExited();
        }
    }

} //namespace view