
#ifndef __view_drag_utils_h__
#define __view_drag_utils_h__

#pragma once

namespace gfx
{
    class Canvas;
    class Point;
    class Size;
}

class SkBitmap;

class OSExchangeData;

namespace view
{

    // Sets the drag image on data_object from the supplied canvas. width/height
    // are the size of the image to use, and the offsets give the location of
    // the hotspot for the drag image.
    void SetDragImageOnDataObject(const gfx::Canvas& canvas,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        OSExchangeData* data_object);

    // Sets the drag image on data_object from the supplied bitmap. width/height
    // are the size of the image to use, and the offsets give the location of
    // the hotspot for the drag image.
    void SetDragImageOnDataObject(const SkBitmap& bitmap,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        OSExchangeData* data_object);

} //namespace view

#endif //__view_drag_utils_h__