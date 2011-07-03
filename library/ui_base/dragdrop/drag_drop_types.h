
#ifndef __ui_base_drag_drop_types_h__
#define __ui_base_drag_drop_types_h__

#pragma once

#include "base/basic_types.h"

namespace ui
{

    class DragDropTypes
    {
    public:
        enum DragOperation
        {
            DRAG_NONE = 0,
            DRAG_MOVE = 1 << 0,
            DRAG_COPY = 1 << 1,
            DRAG_LINK = 1 << 2
        };

        static uint32 DragOperationToDropEffect(int drag_operation);
        static int DropEffectToDragOperation(uint32 effect);
    };

} //namespace ui

#endif //__ui_base_drag_drop_types_h__