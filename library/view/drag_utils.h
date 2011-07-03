
#ifndef __view_drag_utils_h__
#define __view_drag_utils_h__

#pragma once

#include <objidl.h>

#include "base/file_path.h"

namespace gfx
{
    class Canvas;
    class Point;
    class Size;
}

class SkBitmap;

namespace view
{

    // Creates a dragging image to be displayed when the user drags a file from
    // Chrome (via the download manager, for example). The drag image is set into
    // the supplied data_object. 'file_name' can be a full path, but the directory
    // portion will be truncated in the drag image.
    void CreateDragImageForFile(const FilePath& file_name,
        const SkBitmap* icon,
        IDataObject* data_object);

    // Sets the drag image on data_object from the supplied canvas. width/height
    // are the size of the image to use, and the offsets give the location of
    // the hotspot for the drag image.
    void SetDragImageOnDataObject(const gfx::Canvas& canvas,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        IDataObject* data_object);

    // Sets the drag image on data_object from the supplied bitmap. width/height
    // are the size of the image to use, and the offsets give the location of
    // the hotspot for the drag image.
    void SetDragImageOnDataObject(const SkBitmap& bitmap,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        IDataObject* data_object);

} //namespace view

#endif //__view_drag_utils_h__