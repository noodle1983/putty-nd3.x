
#include "drag_utils.h"

#include <shlobj.h>
#include <shobjidl.h>

#include "base/logging.h"
#include "base/utf_string_conversions.h"

#include "gfx/canvas_skia.h"
#include "gfx/font.h"
#include "gfx/gdi_util.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "gfx/skbitmap_operations.h"

#include "SkBitmap.h"

#include "../base/resource_bundle.h"

namespace view
{

    // Maximum width of the link drag image in pixels.
    static const int kLinkDragImageMaxWidth = 200;
    static const int kLinkDragImageVPadding = 3;

    // File dragging pixel measurements
    static const int kFileDragImageMaxWidth = 200;
    static const SkColor kFileDragImageTextColor = SK_ColorBLACK;

    void CreateDragImageForFile(const FilePath& file_name,
        const SkBitmap* icon,
        IDataObject* data_object)
    {
        DCHECK(icon);
        DCHECK(data_object);

        // Set up our text portion
        ResourceBundle& rb = ResourceBundle::GetSharedInstance();
        gfx::Font font = rb.GetFont(ResourceBundle::BaseFont);

        const int width = kFileDragImageMaxWidth;
        // Add +2 here to allow room for the halo.
        const int height = font.GetHeight() + icon->height() +
            kLinkDragImageVPadding + 2;
        gfx::CanvasSkia canvas(width, height, false);

        // Paint the icon.
        canvas.DrawBitmapInt(*icon, (width-icon->width())/2, 0);

        std::wstring name = UTF16ToWide(file_name.BaseName().LossyDisplayName());
        // Paint the file name. We inset it one pixel to allow room for the halo.
        canvas.DrawStringWithHalo(name, font, kFileDragImageTextColor, SK_ColorWHITE,
            1, icon->height()+kLinkDragImageVPadding+1,
            width-2, font.GetHeight(),
            gfx::Canvas::TEXT_ALIGN_CENTER);

        SetDragImageOnDataObject(canvas, gfx::Size(width, height),
            gfx::Point(width/2, kLinkDragImageVPadding), data_object);
    }

    void SetDragImageOnDataObject(const gfx::Canvas& canvas,
        const gfx::Size& size,
        const gfx::Point& cursor_offset,
        IDataObject* data_object)
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
        IDataObject* data_object)
    {
        DCHECK(data_object && !size.IsEmpty());
        // InitializeFromBitmap() doesn't expect an alpha channel and is confused
        // by premultiplied colors, so unpremultiply the bitmap.
        // SetDragImageOnDataObject(HBITMAP) takes ownership of the bitmap.
        HBITMAP bitmap = CreateHBITMAPFromSkBitmap(
            SkBitmapOperations::UnPreMultiply(sk_bitmap));

        // Attach 'bitmap' to the data_object.
        SetDragImageOnDataObject(bitmap, size, cursor_offset, data_object);
    }

} //namespace view