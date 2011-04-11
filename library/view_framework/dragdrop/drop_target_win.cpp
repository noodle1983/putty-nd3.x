
#include "drop_target_win.h"

#include "gfx/point.h"

#include "../view/root_view.h"
#include "../widget/widget.h"
#include "drag_drop_types.h"
#include "os_exchange_data_provider_win.h"

namespace view
{

    DropTargetWin::DropTargetWin(RootView* root_view)
        : DropTarget(root_view->GetWidget()->GetNativeView()),
        helper_(root_view) {}

    DropTargetWin::~DropTargetWin() {}

    void DropTargetWin::ResetTargetViewIfEquals(View* view)
    {
        helper_.ResetTargetViewIfEquals(view);
    }

    DWORD DropTargetWin::OnDragOver(IDataObject* data_object,
        DWORD key_state,
        POINT cursor_position,
        DWORD effect)
    {
        gfx::Point root_view_location(cursor_position.x, cursor_position.y);
        View::ConvertPointToView(NULL, helper_.root_view(), &root_view_location);
        OSExchangeData data(new OSExchangeDataProviderWin(data_object));
        int drop_operation = helper_.OnDragOver(data, root_view_location,
            DragDropTypes::DropEffectToDragOperation(effect));
        return DragDropTypes::DragOperationToDropEffect(drop_operation);
    }

    void DropTargetWin::OnDragLeave(IDataObject* data_object)
    {
        helper_.OnDragExit();
    }

    DWORD DropTargetWin::OnDrop(IDataObject* data_object,
        DWORD key_state,
        POINT cursor_position,
        DWORD effect)
    {
        gfx::Point root_view_location(cursor_position.x, cursor_position.y);
        View::ConvertPointToView(NULL, helper_.root_view(), &root_view_location);

        OSExchangeData data(new OSExchangeDataProviderWin(data_object));
        int drop_operation = DragDropTypes::DropEffectToDragOperation(effect);
        drop_operation = helper_.OnDragOver(data, root_view_location,
            drop_operation);
        drop_operation = helper_.OnDrop(data, root_view_location, drop_operation);
        return DragDropTypes::DragOperationToDropEffect(drop_operation);
    }

} //namespace view