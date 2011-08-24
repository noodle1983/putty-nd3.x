
#include "drop_target_win.h"

#include "view/widget/root_view.h"
#include "view/widget/widget.h"

namespace view
{

    DropTargetWin::DropTargetWin(internal::RootView* root_view)
        : DropTarget(root_view->GetWidget()->GetNativeView()),
        helper_(root_view) {}

    DropTargetWin::~DropTargetWin() {}

    void DropTargetWin::ResetTargetViewIfEquals(View* view)
    {
        helper_.ResetTargetViewIfEquals(view);
    }

    DWORD DropTargetWin::OnDragOver(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        return helper_.OnDragOver(data_object, key_state, cursor_position, effect);
    }

    void DropTargetWin::OnDragLeave(IDataObject* data_object)
    {
        helper_.OnDragLeave();
    }

    DWORD DropTargetWin::OnDrop(IDataObject* data_object,
        DWORD key_state,
        POINTL cursor_position,
        DWORD effect)
    {
        return helper_.OnDrop(data_object, key_state, cursor_position, effect);
    }

} //namespace view