
#include "drop_helper.h"

#include "view/view.h"

namespace view
{

    DropHelper::DropHelper(View* root_view)
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

    DWORD DropHelper::OnDragOver(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        gfx::Point root_view_location(cursor_position.x, cursor_position.y);
        View::ConvertPointToView(NULL, root_view(), &root_view_location);

        View* view = CalculateTargetViewImpl(root_view_location, data_object,
            &deepest_view_);

        if(view != target_view_)
        {
            // 目标视图变化, 通知旧的目标视图退出, 新的目标视图进入.
            NotifyDragLeave();
            target_view_ = view;
            NotifyDragEntered(data_object, key_state, cursor_position, effect);
        }

        return NotifyDragOver(data_object, key_state, cursor_position, effect);
    }

    void DropHelper::OnDragLeave()
    {
        NotifyDragLeave();
        deepest_view_ = target_view_ = NULL;
    }

    DWORD DropHelper::OnDrop(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        View* drop_view = target_view_;
        deepest_view_ = target_view_ = NULL;
        if(!drop_view)
        {
            return DROPEFFECT_NONE;
        }
        
        if(effect == DROPEFFECT_NONE)
        {
            drop_view->DragLeave();
            return DROPEFFECT_NONE;
        }
        
        drop_view->Drop(data_object, key_state,
            cursor_position, &effect);
        return effect;
    }

    View* DropHelper::CalculateTargetView(
        const gfx::Point& root_view_location,
        IDataObject* data_object)
    {
        return CalculateTargetViewImpl(root_view_location,
            data_object, NULL);
    }

    View* DropHelper::CalculateTargetViewImpl(
        const gfx::Point& root_view_location,
        IDataObject* data_object,
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
        while(view && view!=target_view_ && !view->IsEnabled())
        {
            view = view->parent();
        }
        return view;
    }

    void DropHelper::NotifyDragEntered(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        if(!target_view_)
        {
            return;
        }

        target_view_->DragEnter(data_object, key_state,
            cursor_position, &effect);
    }

    DWORD DropHelper::NotifyDragOver(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        if(!target_view_)
        {
            return DROPEFFECT_NONE;
        }

        target_view_->DragOver(key_state, cursor_position, &effect);
        return effect;
    }

    void DropHelper::NotifyDragLeave()
    {
        if(target_view_)
        {
            target_view_->DragLeave();
        }
    }

} //namespace view