
#ifndef __cursors_h__
#define __cursors_h__

#pragma once

#include <windows.h>

enum CursorShape
{
    CURSORSHAPE_CREATE,
    CURSORSHAPE_CREATE_ABOVE,
    CURSORSHAPE_CREATE_BELOW,

    CURSORSHAPE_MOVE,
    CURSORSHAPE_RESIZE_LEFT,
    CURSORSHAPE_RESIZE_RIGHT,
    
    CURSORSHAPE_COUNT
};

extern HCURSOR cursors[CURSORSHAPE_COUNT];
void InitCursors();
void UninitCursors();

#endif //__cursors_h__