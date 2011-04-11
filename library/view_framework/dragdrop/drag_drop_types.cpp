
#include "drag_drop_types.h"

#include <oleidl.h>

int DragDropTypes::DropEffectToDragOperation(uint32 effect)
{
    int drag_operation = DRAG_NONE;
    if(effect & DROPEFFECT_LINK)
    {
        drag_operation |= DRAG_LINK;
    }
    if(effect & DROPEFFECT_COPY)
    {
        drag_operation |= DRAG_COPY;
    }
    if(effect & DROPEFFECT_MOVE)
    {
        drag_operation |= DRAG_MOVE;
    }
    return drag_operation;
}

uint32 DragDropTypes::DragOperationToDropEffect(int drag_operation)
{
    uint32 drop_effect = DROPEFFECT_NONE;
    if(drag_operation & DRAG_LINK)
    {
        drop_effect |= DROPEFFECT_LINK;
    }
    if(drag_operation & DRAG_COPY)
    {
        drop_effect |= DROPEFFECT_COPY;
    }
    if(drag_operation & DRAG_MOVE)
    {
        drop_effect |= DROPEFFECT_MOVE;
    }
    return drop_effect;
}