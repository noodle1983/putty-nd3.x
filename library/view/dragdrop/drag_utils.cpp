
#include "drag_utils.h"

#include <objidl.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "base/logging.h"

#include "gfx/canvas_skia.h"
#include "gfx/gdi_util.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "gfx/skbitmap_operations.h"

#include "SkBitmap.h"

#include "os_exchange_data_provider_win.h"

namespace view
{

    // Maximum width of the link drag image in pixels.
    static const int kLinkDragImageMaxWidth = 200;
    static const int kLinkDragImageVPadding = 3;

    // File dragging pixel measurements
    static const int kFileDragImageMaxWidth = 200;
    static const SkColor kFileDragImageTextColor = SK_ColorBLACK;

    void SetDragImageOnDataObject(const gfx::Canvas& canvas,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        OSExchangeData* data_object)
    {
        SetDragImageOnDataObject(canvas.AsCanvasSkia()->ExtractBitmap(),
            size, cursor_offset, data_object);
    }

    static void SetDragImageOnDataObject(HBITMAP hbitmap,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        IDataObject* data_object)
    {
        IDragSourceHelper* helper = NULL;
        HRESULT rv = CoCreateInstance(CLSID_DragDropHelper, 0,
            CLSCTX_INPROC_SERVER,
            IID_IDragSourceHelper,
            reinterpret_cast<LPVOID*>(&helper));
        if(SUCCEEDED(rv))
        {
            SHDRAGIMAGE sdi;
            sdi.sizeDragImage = size.ToSIZE();
            sdi.crColorKey = 0xFFFFFFFF;
            sdi.hbmpDragImage = hbitmap;
            sdi.ptOffset = cursor_offset.ToPOINT();
            helper->InitializeFromBitmap(&sdi, data_object);
        }
    }

    // Blit the contents of the canvas to a new HBITMAP. It is the caller's
    // responsibility to release the |bits| buffer.
    static HBITMAP CreateHBITMAPFromSkBitmap(const SkBitmap& sk_bitmap)
    {
        HDC screen_dc = GetDC(NULL);
        BITMAPINFOHEADER header;
        gfx::CreateBitmapHeader(sk_bitmap.width(), sk_bitmap.height(), &header);
        void* bits;
        HBITMAP bitmap = CreateDIBSection(screen_dc,
            reinterpret_cast<BITMAPINFO*>(&header),
            DIB_RGB_COLORS, &bits, NULL, 0);
        DCHECK(sk_bitmap.rowBytes() == sk_bitmap.width()*4);
        SkAutoLockPixels lock(sk_bitmap);
        memcpy(bits, sk_bitmap.getPixels(),
            sk_bitmap.height()*sk_bitmap.rowBytes());
        ReleaseDC(NULL, screen_dc);
        return bitmap;
    }

    void SetDragImageOnDataObject(const SkBitmap& sk_bitmap,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        OSExchangeData* data_object)
    {
        DCHECK(data_object && !size.IsEmpty());
        // InitializeFromBitmap() doesn't expect an alpha channel and is confused
        // by premultiplied colors, so unpremultiply the bitmap.
        // SetDragImageOnDataObject(HBITMAP) takes ownership of the bitmap.
        HBITMAP bitmap = CreateHBITMAPFromSkBitmap(
            SkBitmapOperations::UnPreMultiply(sk_bitmap));

        // Attach 'bitmap' to the data_object.
        SetDragImageOnDataObject(bitmap, size, cursor_offset,
            OSExchangeDataProviderWin::GetIDataObject(*data_object));
    }

} //namespace view