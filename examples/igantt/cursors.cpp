
#include "cursors.h"

#include "resource.h"

namespace
{
    static const int cursor_ids[CURSORSHAPE_COUNT] =
    {
        IDC_CURSOR_CREATE,
        IDC_CURSOR_CREATE_ABOVE,
        IDC_CURSOR_CREATE_BELOW,
        IDC_CURSOR_MOVE,
        IDC_CURSOR_RESIZE_LEFT,
        IDC_CURSOR_RESIZE_RIGHT
    };
}

HCURSOR cursors[CURSORSHAPE_COUNT];

extern "C" IMAGE_DOS_HEADER __ImageBase;

void InitCursors()
{
    HMODULE resources_data_ = reinterpret_cast<HMODULE>(&__ImageBase);
    for(int i=0; i<CURSORSHAPE_COUNT; ++i)
    {
        cursors[i] = LoadCursor(resources_data_,
            MAKEINTRESOURCE(cursor_ids[i]));
    }
}

void UninitCursors()
{
    for(int i=0; i<CURSORSHAPE_COUNT; ++i)
    {
        DestroyCursor(cursors[i]);
        cursors[i] = NULL;
    }
}