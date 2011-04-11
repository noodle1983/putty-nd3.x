
#ifndef __gfx_canvas_h__
#define __gfx_canvas_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"
#include "base/string16.h"

#include "skia/ext/platform_canvas.h"

namespace gfx
{

    class Brush;
    class CanvasSkia;
    class Font;
    class Point;
    class Rect;
    class Transform;
    typedef unsigned int TextureID;

    class Canvas
    {
    public:
        // 指定DrawStringInt方法渲染文本的对齐方式.
        enum
        {
            TEXT_ALIGN_LEFT = 1,
            TEXT_ALIGN_CENTER = 2,
            TEXT_ALIGN_RIGHT = 4,
            TEXT_VALIGN_TOP = 8,
            TEXT_VALIGN_MIDDLE = 16,
            TEXT_VALIGN_BOTTOM = 32,

            // 指定多行文本.
            MULTI_LINE = 64,

            // 缺省情况下DrawStringInt不特别处理前缀('&')字符. 也就是字符串"&foo"被
            // 渲染成"&foo". 当渲染带有记忆前缀字符的资源文本时, 前缀可以处理成下划
            // 线(SHOW_PREFIX)或者完全不渲染(HIDE_PREFIX).
            SHOW_PREFIX = 128,
            HIDE_PREFIX = 256,

            // 阻止省略号.
            NO_ELLIPSIS = 512,

            // 指定单词可以被新行切割. 只能与MULTI_LINE一起工作.
            CHARACTER_BREAK = 1024,

            // 指示DrawStringInt()使用RTL方向性渲染文本. 大多数情况下没必要传递这个
            // 标志位, 因为文本的方向性信息会以特殊Unicode字符的形式嵌入在字符串中.
            // 但是如果本地是LTR, 我们不会插入方向性字符, 因为有些平台(例如没有安装
            // RTL字体的英文版Windows XP)不支持这些字符. 所以, 当本地是LTR时, 这个
            // 标志可用于渲染RTL方向性的文本.
            FORCE_RTL_DIRECTIONALITY = 2048,
        };

        virtual ~Canvas() {}

        // 创建一个空的画布. 使用前必须初始化.
        static Canvas* CreateCanvas();

        // 创建特定大小的画布.
        static Canvas* CreateCanvas(int width, int height, bool is_opaque);

        // 在栈上保存一份绘图状态的拷贝, 在配对调用Restore()之前, 所有的操作
        // 都在这份拷贝上.
        virtual void Save() = 0;

        // 和Save()一样, 只是绘制在一个图层上, 在调用Restore()时以指定的alpha值
        // 与画布融合.
        // |layer_bounds|是图层在当前坐标变换下的范围.
        virtual void SaveLayerAlpha(uint8 alpha) = 0;
        virtual void SaveLayerAlpha(uint8 alpha, const Rect& layer_bounds) = 0;

        // 用于在调用Save*()后恢复绘图状态. Restore()多于Save*()时会发生错误.
        virtual void Restore() = 0;

        // 函数封装接受整数参数.
        // 如果裁剪为空返回true. 详细参见clipRect.
        virtual bool ClipRectInt(int x, int y, int w, int h) = 0;

        // 函数封装接受整数参数.
        // 详细参见translate().
        virtual void TranslateInt(int x, int y) = 0;

        // 函数封装接受整数参数.
        // 详细参见scale().
        virtual void ScaleInt(int x, int y) = 0;

        // 用特定颜色以SkXfermode::kSrcOver_Mode方式填充指定区域.
        virtual void FillRectInt(const SkColor& color,
            int x, int y, int w, int h) = 0;

        // 用特定颜色以mode方式填充指定区域.
        virtual void FillRectInt(const SkColor& color,
            int x, int y, int w, int h, SkXfermode::Mode mode) = 0;

        // 用特定画刷填充指定区域.
        virtual void FillRectInt(const Brush* brush,
            int x, int y, int w, int h) = 0;

        // 在特定区域用给定颜色和SkXfermode::kSrcOver_Mode变化模式绘制矩形框.
        //
        // 注意: 如果你只是需要绘制一根线, 请使用DrawLineInt.
        virtual void DrawRectInt(const SkColor& color,
            int x, int y, int w, int h) = 0;

        // 在特定区域用给定颜色和变化模式绘制矩形框.
        //
        // 注意: 如果你只是需要绘制一根线, 请使用DrawLineInt.
        virtual void DrawRectInt(const SkColor& color,
            int x, int y, int w, int h,
            SkXfermode::Mode mode) = 0;

        // 用特定绘图参数绘制矩形.
        virtual void DrawRectInt(int x, int y, int w, int h,
            const SkPaint& paint) = 0;

        // 用特定颜色绘制线.
        virtual void DrawLineInt(const SkColor& color,
            int x1, int y1, int x2, int y2) = 0;

        // 在特定位置绘制位图. 位图的左上角渲染在那个位置.
        virtual void DrawBitmapInt(const SkBitmap& bitmap, int x, int y) = 0;

        // 在特定位置用给定的绘图参数绘制位图. 位图的左上角渲染在那个位置.
        virtual void DrawBitmapInt(const SkBitmap& bitmap,
            int x, int y, const SkPaint& paint) = 0;

        // 绘制部分位图到特定位置. src参数对应位图的区域, 绘制到dest坐标定义的
        // 区域.
        //
        // 如果源的宽或高和目的的不一样, 图像将会缩放. 缩小时, 强烈建议你对位图
        // 调用buildMipMap(false)以确保它有一个缩小贴图(mipmap), 这样可以有高质
        // 量的输出. 设置|filter|可以使用位图的过滤, 否则重采样使用的是最近邻算法.
        //
        // SkPaint提供可选定制.
        virtual void DrawBitmapInt(const SkBitmap& bitmap,
            int src_x, int src_y, int src_w, int src_h,
            int dest_x, int dest_y, int dest_w, int dest_h,
            bool filter) = 0;
        virtual void DrawBitmapInt(const SkBitmap& bitmap,
            int src_x, int src_y, int src_w, int src_h,
            int dest_x, int dest_y, int dest_w, int dest_h,
            bool filter, const SkPaint& paint) = 0;

        // 用特定颜色、字体在给定的位置绘制文本. 文本水平方向左对齐, 垂直方向
        // 居中对齐, 在区域中裁剪. 如果文本太大, 会截取并在末尾添加'...'.
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            int x, int y, int w, int h) = 0;
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            const Rect& display_rect) = 0;

        // 用特定颜色、字体在给定的位置绘制文本. 最后一个参数指定文本渲染的方式.
        // 可以是TEXT_ALIGN_CENTER、TEXT_ALIGN_RIGHT或TEXT_ALIGN_LEFT中的一个.
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            int x, int y, int w, int h,
            int flags) = 0;

        // 绘制打点的灰色矩形用于显示焦点.
        virtual void DrawFocusRect(int x, int y, int width, int height) = 0;

        // 在特定区域平铺图像.
        virtual void TileImageInt(const SkBitmap& bitmap,
            int x, int y, int w, int h) = 0;
        virtual void TileImageInt(const SkBitmap& bitmap,
            int src_x, int src_y,
            int dest_x, int dest_y, int w, int h) = 0;

        // 返回本地绘图环境用于平台相关的绘图操作. 必须与EndPlatformPaint()配对调用.
        virtual HDC BeginPlatformPaint() = 0;

        // 表明结束使用本地绘图环境进行平台相关的绘图, 设备是BeginPlatformPaint()
        // 返回的.
        virtual void EndPlatformPaint() = 0;

        // 对画布应用变换.
        virtual void ConcatTransform(const Transform& transform) = 0;

        // 创建一个纹理用于加速绘图.
        virtual TextureID GetTextureID() = 0;

        // 获取底层SkCanvas的方法.
        virtual CanvasSkia* AsCanvasSkia();
        virtual const CanvasSkia* AsCanvasSkia() const;
    };

    class CanvasPaint
    {
    public:
        virtual ~CanvasPaint() {}

        // 创建一个画布, 销毁时绘制到|view|. 画布的大小等于|view|的客户区.
        static CanvasPaint* CreateCanvasPaint(HWND view);

        // 如果画布有一个需要重绘的无效矩形则返回true.
        virtual bool IsValid() const = 0;

        // 返回无效矩形.
        virtual Rect GetInvalidRect() const = 0;

        // 返回底层的画布.
        virtual Canvas* AsCanvas() = 0;
    };

} //namespace gfx

#endif //__gfx_canvas_h__