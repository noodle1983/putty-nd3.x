
#include "vector_platform_device_win.h"

#include <windows.h>

#include "skia_utils_win.h"
#include "SkUtils.h"

#include "bitmap_platform_device_win.h"

namespace skia
{

    SkDevice* VectorPlatformDeviceFactory::newDevice(SkCanvas* unused,
        SkBitmap::Config config,
        int width, int height,
        bool isOpaque,
        bool isForLayer)
    {
        SkASSERT(config == SkBitmap::kARGB_8888_Config);
        return CreateDevice(width, height, isOpaque, NULL);
    }

    //static
    SkDevice* VectorPlatformDeviceFactory::CreateDevice(int width, int height,
        bool is_opaque, HANDLE shared_section)
    {
        if(!is_opaque)
        {
            // TODO: http://crbug.com/18382 当恢复一个半透明层时, 比如合并它, 我们需要
            // 对它进行光栅化, 因为GDI除AlphaBlend()以为的函数都不支持透明度. 目前当
            // VectorCanvas想要调用saveLayers()时, 会创建一个BitmapPlatformDevice. 保存
            // 层的方式是创建一个基于EMF的VectorDevice, 用它来记录绘图过程. 回放到位图
            // 的时候, 使用打印机的dpi替代设备的dpi(低得多).
            return BitmapPlatformDevice::create(width, height,
                is_opaque, shared_section);
        }

        // TODO: http://crbug.com/18383 当使用的dpi较低时, 看是否有必要把分辨率
        // 提高10倍(可以是任意值)来增加渲染精度(打印的时候). 能这样做是因为我们
        // 的输入是浮点数, 而GDI函数使用的是整数. 想法就是矩阵先自乘这个系数,
        // 传递给SkScalarRound(value)的每个SkScalar都变为SkScalarRound(value*10).
        // Safari的文本渲染已经是这么做了.
        SkASSERT(shared_section);
        PlatformDevice* device = VectorPlatformDevice::create(
            reinterpret_cast<HDC>(shared_section), width, height);
        return device;
    }

    static void FillBitmapInfoHeader(int width, int height, BITMAPINFOHEADER* hdr)
    {
        hdr->biSize = sizeof(BITMAPINFOHEADER);
        hdr->biWidth = width;
        hdr->biHeight = -height; // 负号表示自上而下的位图.
        hdr->biPlanes = 1;
        hdr->biBitCount = 32;
        hdr->biCompression = BI_RGB; // 不压缩.
        hdr->biSizeImage = 0;
        hdr->biXPelsPerMeter = 1;
        hdr->biYPelsPerMeter = 1;
        hdr->biClrUsed = 0;
        hdr->biClrImportant = 0;
    }

    VectorPlatformDevice* VectorPlatformDevice::create(HDC dc, int width,
        int height)
    {
        InitializeDC(dc);

        // 把SkBitmap选入到设备环境中.
        SkBitmap bitmap;
        HGDIOBJ selected_bitmap = GetCurrentObject(dc, OBJ_BITMAP);
        bool succeeded = false;
        if(selected_bitmap != NULL)
        {
            BITMAP bitmap_data;
            if(GetObject(selected_bitmap, sizeof(BITMAP), &bitmap_data) ==
                sizeof(BITMAP))
            {
                // 设备环境中已经有位图, 把SkBitmap附加到这个位图.
                // 警告: 如果位图从HDC选出, VectorPlatformDevice无法检测到, 所以
                // 位图会被释放, 此时SkBitmap还对它有引用. 需要小心.
                if(width==bitmap_data.bmWidth && height==bitmap_data.bmHeight)
                {
                    bitmap.setConfig(SkBitmap::kARGB_8888_Config,
                        bitmap_data.bmWidth,
                        bitmap_data.bmHeight,
                        bitmap_data.bmWidthBytes);
                    bitmap.setPixels(bitmap_data.bmBits);
                    succeeded = true;
                }
            }
        }

        if(!succeeded)
        {
            bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
        }

        return new VectorPlatformDevice(dc, bitmap);
    }

    VectorPlatformDevice::VectorPlatformDevice(HDC dc, const SkBitmap& bitmap)
        : PlatformDevice(bitmap),
        hdc_(dc),
        previous_brush_(NULL),
        previous_pen_(NULL),
        alpha_blend_used_(false)
    {
        transform_.reset();
    }

    VectorPlatformDevice::~VectorPlatformDevice()
    {
        SkASSERT(previous_brush_ == NULL);
        SkASSERT(previous_pen_ == NULL);
    }


    void VectorPlatformDevice::drawPaint(const SkDraw& draw, const SkPaint& paint)
    {
        // TODO: 忽略当前的变换矩阵.
        SkRect rect;
        rect.fLeft = 0;
        rect.fTop = 0;
        rect.fRight = SkIntToScalar(width() + 1);
        rect.fBottom = SkIntToScalar(height() + 1);
        drawRect(draw, rect, paint);
    }

    void VectorPlatformDevice::drawPoints(const SkDraw& draw,
        SkCanvas::PointMode mode,
        size_t count,
        const SkPoint pts[],
        const SkPaint& paint)
    {
        if(!count)
        {
            return;
        }

        if(mode == SkCanvas::kPoints_PointMode)
        {
            SkASSERT(false);
            return;
        }

        SkPaint tmp_paint(paint);
        tmp_paint.setStyle(SkPaint::kStroke_Style);

        // 用路径替代.
        SkPath path;
        switch(mode)
        {
        case SkCanvas::kLines_PointMode:
            if(count % 2)
            {
                SkASSERT(false);
                return;
            }
            for(size_t i=0; i<count/2; ++i)
            {
                path.moveTo(pts[2 * i]);
                path.lineTo(pts[2 * i + 1]);
            }
            break;
        case SkCanvas::kPolygon_PointMode:
            path.moveTo(pts[0]);
            for(size_t i=1; i<count; ++i)
            {
                path.lineTo(pts[i]);
            }
            break;
        default:
            SkASSERT(false);
            return;
        }
        // 绘制计算出的路径.
        drawPath(draw, path, tmp_paint);
    }

    void VectorPlatformDevice::drawRect(const SkDraw& draw,
        const SkRect& rect, const SkPaint& paint)
    {
        if(paint.getPathEffect())
        {
            // 用路径替代.
            SkPath path_orginal;
            path_orginal.addRect(rect);

            // 对矩形应用路径效果.
            SkPath path_modified;
            paint.getFillPath(path_orginal, &path_modified);

            // 从临时SkPaint对象移除路径效果.
            SkPaint paint_no_effet(paint);
            SkSafeUnref(paint_no_effet.setPathEffect(NULL));

            // 绘制计算出的路径.
            drawPath(draw, path_modified, paint_no_effet);
            return;
        }

        if(!ApplyPaint(paint))
        {
            return;
        }
        HDC dc = getBitmapDC();
        if(!Rectangle(dc, SkScalarRound(rect.fLeft),
            SkScalarRound(rect.fTop),
            SkScalarRound(rect.fRight),
            SkScalarRound(rect.fBottom)))
        {
            SkASSERT(false);
        }
        Cleanup();
    }

    void VectorPlatformDevice::drawPath(const SkDraw& draw,
        const SkPath& path, const SkPaint& paint)
    {
        if(paint.getPathEffect())
        {
            // 应用路径效果.
            SkPath path_modified;
            paint.getFillPath(path, &path_modified);

            // 从临时SkPaint对象移除路径效果.
            SkPaint paint_no_effet(paint);
            SkSafeUnref(paint_no_effet.setPathEffect(NULL));

            // 绘制计算出的路径.
            drawPath(draw, path_modified, paint_no_effet);
            return;
        }

        if(!ApplyPaint(paint))
        {
            return;
        }
        HDC dc = getBitmapDC();
        PlatformDevice::LoadPathToDC(dc, path);
        switch(paint.getStyle())
        {
        case SkPaint::kFill_Style:
            {
                BOOL res = StrokeAndFillPath(dc);
                SkASSERT(res != 0);
                break;
            }
        case SkPaint::kStroke_Style:
            {
                BOOL res = StrokePath(dc);
                SkASSERT(res != 0);
                break;
            }
        case SkPaint::kStrokeAndFill_Style:
            {
                BOOL res = StrokeAndFillPath(dc);
                SkASSERT(res != 0);
                break;
            }
        default:
            SkASSERT(false);
            break;
        }
        Cleanup();
    }

    void VectorPlatformDevice::drawBitmap(const SkDraw& draw,
        const SkBitmap& bitmap,
        const SkMatrix& matrix,
        const SkPaint& paint)
    {
        // 加载临时的矩阵, 对位图平移、旋转和缩放.
        SkMatrix actual_transform(transform_);
        actual_transform.preConcat(matrix);
        LoadTransformToDC(hdc_, actual_transform);

        InternalDrawBitmap(bitmap, 0, 0, paint);

        // 恢复原始的矩阵.
        LoadTransformToDC(hdc_, transform_);
    }

    void VectorPlatformDevice::drawSprite(const SkDraw& draw,
        const SkBitmap& bitmap,
        int x, int y,
        const SkPaint& paint)
    {
        SkMatrix identity;
        identity.reset();
        LoadTransformToDC(hdc_, identity);

        InternalDrawBitmap(bitmap, x, y, paint);

        // 恢复原始的矩阵.
        LoadTransformToDC(hdc_, transform_);
    }

    void VectorPlatformDevice::drawText(const SkDraw& draw,
        const void* text,
        size_t byteLength,
        SkScalar x,
        SkScalar y,
        const SkPaint& paint)
    {
        // 不要在代码中使用本函数.
        SkASSERT(false);
    }

    void VectorPlatformDevice::drawPosText(const SkDraw& draw,
        const void* text,
        size_t len,
        const SkScalar pos[],
        SkScalar constY,
        int scalarsPerPos,
        const SkPaint& paint)
    {
        // 不要在代码中使用本函数.
        SkASSERT(false);
    }

    void VectorPlatformDevice::drawTextOnPath(const SkDraw& draw,
        const void* text,
        size_t len,
        const SkPath& path,
        const SkMatrix* matrix,
        const SkPaint& paint)
    {
        // 不要在代码中使用本函数.
        SkASSERT(false);
    }

    void VectorPlatformDevice::drawVertices(const SkDraw& draw,
        SkCanvas::VertexMode vmode,
        int vertexCount,
        const SkPoint vertices[],
        const SkPoint texs[],
        const SkColor colors[],
        SkXfermode* xmode,
        const uint16_t indices[],
        int indexCount,
        const SkPaint& paint)
    {
        // 不要在代码中使用本函数.
        SkASSERT(false);
    }

    void VectorPlatformDevice::drawDevice(const SkDraw& draw,
        SkDevice* device,
        int x,
        int y,
        const SkPaint& paint)
    {
        // TODO: http://b/1183870 如果是矢量设备, 使用打印机的dpi回放EMF缓冲.
        drawSprite(draw, device->accessBitmap(false), x, y, paint);
    }

    bool VectorPlatformDevice::ApplyPaint(const SkPaint& paint)
    {
        // 注意: 目标是转换SkPaint的状态到HDC的状态. 函数不执行SkPaint的绘图命令.
        // 会在drawPaint()中执行.

        SkPaint::Style style = paint.getStyle();
        if(!paint.getAlpha())
        {
            style = SkPaint::kStyleCount;
        }

        switch(style)
        {
        case SkPaint::kFill_Style:
            if(!CreateBrush(true, paint) || !CreatePen(false, paint))
            {
                return false;
            }
            break;
        case SkPaint::kStroke_Style:
            if(!CreateBrush(false, paint) || !CreatePen(true, paint))
            {
                return false;
            }
            break;
        case SkPaint::kStrokeAndFill_Style:
            if(!CreateBrush(true, paint) || !CreatePen(true, paint))
            {
                return false;
            }
            break;
        default:
            if(!CreateBrush(false, paint) || !CreatePen(false, paint))
            {
                return false;
            }
            break;
        }

        /*
        getFlags();
        isAntiAlias();
        isDither()
        isLinearText()
        isSubpixelText()
        isUnderlineText()
        isStrikeThruText()
        isFakeBoldText()
        isDevKernText()
        isFilterBitmap()

        // Skia's text is not used. This should be fixed.
        getTextAlign()
        getTextScaleX()
        getTextSkewX()
        getTextEncoding()
        getFontMetrics()
        getFontSpacing()
        */

        // BUG 1094907: Implement shaders. Shaders currently in use:
        //  SkShader::CreateBitmapShader
        //  SkGradientShader::CreateRadial
        //  SkGradientShader::CreateLinear
        // SkASSERT(!paint.getShader());

        // http://b/1106647 Implement loopers and mask filter. Looper currently in
        // use:
        //   SkBlurDrawLooper is used for shadows.
        // SkASSERT(!paint.getLooper());
        // SkASSERT(!paint.getMaskFilter());

        // http://b/1165900 Implement xfermode.
        // SkASSERT(!paint.getXfermode());

        // The path effect should be processed before arriving here.
        SkASSERT(!paint.getPathEffect());

        // These aren't used in the code. Verify this assumption.
        SkASSERT(!paint.getColorFilter());
        SkASSERT(!paint.getRasterizer());
        // Reuse code to load Win32 Fonts.
        SkASSERT(!paint.getTypeface());

        return true;
    }

    void VectorPlatformDevice::setMatrixClip(const SkMatrix& transform,
        const SkRegion& region, const SkClipStack&)
    {
        transform_ = transform;
        LoadTransformToDC(hdc_, transform_);
        clip_region_ = region;
        if(!clip_region_.isEmpty())
        {
            LoadClipRegion();
        }
    }

    void VectorPlatformDevice::drawToHDC(HDC dc, int x, int y,
        const RECT* src_rect)
    {
        SkASSERT(false);
    }

    void VectorPlatformDevice::LoadClipRegion()
    {
        SkMatrix t;
        t.reset();
        LoadClippingRegionToDC(hdc_, clip_region_, t);
    }

    bool VectorPlatformDevice::CreateBrush(bool use_brush, COLORREF color)
    {
        SkASSERT(previous_brush_ == NULL);
        // 在对EMF缓冲绘图时不能使用SetDCBrushColor()或者DC_BRUSH.
        // SetDCBrushColor()调用根本不会被记录下来, DC_BRUSH会被WHITE_BRUSH替换.

        if(!use_brush)
        {
            // 设置透明背景.
            if(0 == SetBkMode(hdc_, TRANSPARENT))
            {
                SkASSERT(false);
                return false;
            }

            // 选择空画刷.
            previous_brush_ = SelectObject(GetStockObject(NULL_BRUSH));
            return previous_brush_ != NULL;
        }

        // 设置背景不透明.
        if(0 == SetBkMode(hdc_, OPAQUE))
        {
            SkASSERT(false);
            return false;
        }

        // 创建并选择画刷.
        previous_brush_ = SelectObject(CreateSolidBrush(color));
        return previous_brush_ != NULL;
    }

    bool VectorPlatformDevice::CreatePen(bool use_pen,
        COLORREF color,
        int stroke_width,
        float stroke_miter,
        DWORD pen_style)
    {
        SkASSERT(previous_pen_ == NULL);
        // 在对EMF缓冲绘图时不能使用SetDCPenColor()或者DC_PEN.
        // SetDCPenColor()调用根本不会被记录下来, DC_PEN会被BLACK_PEN替换.

        // 没有画笔的情况.
        if(!use_pen)
        {
            previous_pen_ = SelectObject(GetStockObject(NULL_PEN));
            return previous_pen_ != NULL;
        }

        // 如果笔触宽度为0, 使用固有的画笔.
        if(stroke_width == 0)
        {
            // 使用正确的颜色创建画笔.
            previous_pen_ = SelectObject(::CreatePen(PS_SOLID, 0, color));
            return previous_pen_ != NULL;
        }

        // 加载自定义画笔.
        LOGBRUSH brush;
        brush.lbStyle = BS_SOLID;
        brush.lbColor = color;
        brush.lbHatch = 0;
        HPEN pen = ExtCreatePen(pen_style, stroke_width, &brush, 0, NULL);
        SkASSERT(pen != NULL);
        previous_pen_ = SelectObject(pen);
        if(previous_pen_ == NULL)
        {
            return false;
        }

        if(!SetMiterLimit(hdc_, stroke_miter, NULL))
        {
            SkASSERT(false);
            return false;
        }
        return true;
    }

    void VectorPlatformDevice::Cleanup()
    {
        if(previous_brush_)
        {
            HGDIOBJ result = SelectObject(previous_brush_);
            previous_brush_ = NULL;
            if(result)
            {
                BOOL res = DeleteObject(result);
                SkASSERT(res != 0);
            }
        }
        if(previous_pen_)
        {
            HGDIOBJ result = SelectObject(previous_pen_);
            previous_pen_ = NULL;
            if(result)
            {
                BOOL res = DeleteObject(result);
                SkASSERT(res != 0);
            }
        }
        // 从设备环境中移除所有加载的路径.
        AbortPath(hdc_);
    }

    HGDIOBJ VectorPlatformDevice::SelectObject(HGDIOBJ object)
    {
        HGDIOBJ result = ::SelectObject(hdc_, object);
        SkASSERT(result != HGDI_ERROR);
        if(result == HGDI_ERROR)
        {
            return NULL;
        }
        return result;
    }

    bool VectorPlatformDevice::CreateBrush(bool use_brush, const SkPaint& paint)
    {
        // 对于透明色不要使用画刷.
        if(paint.getAlpha() == 0)
        {
            use_brush = false;
        }

        return CreateBrush(use_brush, SkColorToCOLORREF(paint.getColor()));
    }

    bool VectorPlatformDevice::CreatePen(bool use_pen, const SkPaint& paint)
    {
        // 对于透明色不要使用画笔.
        if(paint.getAlpha() == 0)
        {
            use_pen = false;
        }

        DWORD pen_style = PS_GEOMETRIC | PS_SOLID;
        switch(paint.getStrokeJoin())
        {
        case SkPaint::kMiter_Join:
            // Connects path segments with a sharp join.
            pen_style |= PS_JOIN_MITER;
            break;
        case SkPaint::kRound_Join:
            // Connects path segments with a round join.
            pen_style |= PS_JOIN_ROUND;
            break;
        case SkPaint::kBevel_Join:
            // Connects path segments with a flat bevel join.
            pen_style |= PS_JOIN_BEVEL;
            break;
        default:
            SkASSERT(false);
            break;
        }
        switch(paint.getStrokeCap())
        {
        case SkPaint::kButt_Cap:
            // Begin/end contours with no extension.
            pen_style |= PS_ENDCAP_FLAT;
            break;
        case SkPaint::kRound_Cap:
            // Begin/end contours with a semi-circle extension.
            pen_style |= PS_ENDCAP_ROUND;
            break;
        case SkPaint::kSquare_Cap:
            // Begin/end contours with a half square extension.
            pen_style |= PS_ENDCAP_SQUARE;
            break;
        default:
            SkASSERT(false);
            break;
        }

        return CreatePen(use_pen,
            SkColorToCOLORREF(paint.getColor()),
            SkScalarRound(paint.getStrokeWidth()),
            paint.getStrokeMiter(),
            pen_style);
    }

    void VectorPlatformDevice::InternalDrawBitmap(const SkBitmap& bitmap,
        int x, int y, const SkPaint& paint)
    {
        unsigned char alpha = paint.getAlpha();
        if(alpha == 0)
        {
            return;
        }

        bool is_translucent;
        if(alpha != 255)
        {
            // ApplyPaint需要非透明色.
            SkPaint tmp_paint(paint);
            tmp_paint.setAlpha(255);
            if(!ApplyPaint(tmp_paint))
            {
                return;
            }
            is_translucent = true;
        }
        else
        {
            if(!ApplyPaint(paint))
            {
                return;
            }
            is_translucent = false;
        }
        int src_size_x = bitmap.width();
        int src_size_y = bitmap.height();
        if(!src_size_x || !src_size_y)
        {
            return;
        }

        // Create a BMP v4 header that we can serialize. We use the shared "V3"
        // fillter to fill the stardard items, then add in the "V4" stuff we want.
        BITMAPV4HEADER bitmap_header;
        memset(&bitmap_header, 0, sizeof(BITMAPV4HEADER));
        FillBitmapInfoHeader(src_size_x, src_size_y,
            reinterpret_cast<BITMAPINFOHEADER*>(&bitmap_header));
        bitmap_header.bV4Size = sizeof(BITMAPV4HEADER);
        bitmap_header.bV4RedMask   = 0x00ff0000;
        bitmap_header.bV4GreenMask = 0x0000ff00;
        bitmap_header.bV4BlueMask  = 0x000000ff;
        bitmap_header.bV4AlphaMask = 0xff000000;

        HDC dc = getBitmapDC();
        SkAutoLockPixels lock(bitmap);
        SkASSERT(bitmap.getConfig() == SkBitmap::kARGB_8888_Config);
        const uint32_t* pixels = static_cast<const uint32_t*>(bitmap.getPixels());
        if(pixels == NULL)
        {
            SkASSERT(false);
            return;
        }

        if(!is_translucent)
        {
            int row_length = bitmap.rowBytesAsPixels();
            // There is no quick way to determine if an image is opaque.
            for(int y2=0; y2<src_size_y; ++y2)
            {
                for(int x2=0; x2<src_size_x; ++x2)
                {
                    if(SkColorGetA(pixels[(y2 * row_length) + x2]) != 255)
                    {
                        is_translucent = true;
                        y2 = src_size_y;
                        break;
                    }
                }
            }
        }

        BITMAPINFOHEADER hdr;
        FillBitmapInfoHeader(src_size_x, src_size_y, &hdr);
        if(is_translucent)
        {
            // The image must be loaded as a bitmap inside a device context.
            HDC bitmap_dc = ::CreateCompatibleDC(dc);
            void* bits = NULL;
            HBITMAP hbitmap = ::CreateDIBSection(
                bitmap_dc, reinterpret_cast<const BITMAPINFO*>(&hdr),
                DIB_RGB_COLORS, &bits, NULL, 0);

            // static cast to a char so we can do byte ptr arithmatic to
            // get the offset.
            unsigned char* dest_buffer = static_cast<unsigned char *>(bits);

            // We will copy row by row to avoid having to worry about
            // the row strides being different.
            const int dest_row_size = hdr.biBitCount / 8 * hdr.biWidth;
            for(int row=0; row<bitmap.height(); ++row)
            {
                int dest_offset = row * dest_row_size;
                // pixels_offset in terms of pixel count.
                int src_offset = row * bitmap.rowBytesAsPixels();
                memcpy(dest_buffer+dest_offset, pixels+src_offset, dest_row_size);
            }
            SkASSERT(hbitmap);
            HGDIOBJ old_bitmap = ::SelectObject(bitmap_dc, hbitmap);

            // After some analysis of IE7's behavior, this is the thing to do. I was
            // sure IE7 was doing so kind of bitmasking due to the way translucent image
            // where renderered but after some windbg tracing, it is being done by the
            // printer driver after all (mostly HP printers). IE7 always use AlphaBlend
            // for bitmasked images. The trick seems to switch the stretching mode in
            // what the driver expects.
            DWORD previous_mode = GetStretchBltMode(dc);
            BOOL result = SetStretchBltMode(dc, COLORONCOLOR);
            SkASSERT(result);
            // Note that this function expect premultiplied colors (!)
            BLENDFUNCTION blend_function = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
            result = GdiAlphaBlend(dc,
                x, y, // Destination origin.
                src_size_x, src_size_y, // Destination size.
                bitmap_dc,
                0, 0, // Source origin.
                src_size_x, src_size_y, // Source size.
                blend_function);
            SkASSERT(result);
            result = SetStretchBltMode(dc, previous_mode);
            SkASSERT(result);

            alpha_blend_used_ = true;

            ::SelectObject(bitmap_dc, static_cast<HBITMAP>(old_bitmap));
            DeleteObject(hbitmap);
            DeleteDC(bitmap_dc);
        }
        else
        {
            BOOL result = StretchDIBits(dc,
                x, y, // Destination origin.
                src_size_x, src_size_y,
                0, 0, // Source origin.
                src_size_x, src_size_y, // Source size.
                pixels,
                reinterpret_cast<const BITMAPINFO*>(&hdr),
                DIB_RGB_COLORS,
                SRCCOPY);
            SkASSERT(result);
        }
        Cleanup();
    }

} //namespace skia