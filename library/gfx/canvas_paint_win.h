
#ifndef __gfx_canvas_paint_win_h__
#define __gfx_canvas_paint_win_h__

#pragma once

#include "skia/ext/platform_canvas.h"

namespace gfx
{

    // CanvasPaintT是Windows平台WM_PAINT消息处理的辅助类. 它在init/destruction
    // 中分别进行了BeginPaint/EndPaint, 会创建正确大小的位图和画布, 且针对脏矩形
    // 进行平移. 位图在析构时自动的绘制到屏幕.
    //
    // 在绘图之前比较调用isEmpty确定是否有东西需要绘制. 脏矩形有时候是空的, 位图
    // 函数不接受这种矩形. 此时, 调用者不应该绘图.
    //
    // 因此, 你需要这么做:
    //     case WM_PAINT: {
    //       skia::PlatformCanvasPaint canvas(hwnd);
    //       if(!canvas.isEmpty()) {
    //         ... paint to the canvas ...
    //       }
    //       return 0;
    //     }
    template<class T>
    class CanvasPaintT : public T
    {
    public:
        // 构造函数假定画布是不透明的.
        explicit CanvasPaintT(HWND hwnd) : hwnd_(hwnd), paint_dc_(NULL),
            for_paint_(true)
        {
            memset(&ps_, 0, sizeof(ps_));
            initPaint(true);
        }

        CanvasPaintT(HWND hwnd, bool opaque) : hwnd_(hwnd), paint_dc_(NULL),
            for_paint_(true)
        {
            memset(&ps_, 0, sizeof(ps_));
            initPaint(opaque);
        }

        // 为dc上的绘图区创建一个CanvasPaintT. 不调用BeginPaint/EndPaint.
        CanvasPaintT(HDC dc, bool opaque, int x, int y, int w, int h)
            : hwnd_(NULL), paint_dc_(dc), for_paint_(false)
        {
            memset(&ps_, 0, sizeof(ps_));
            ps_.rcPaint.left = x;
            ps_.rcPaint.right = x + w;
            ps_.rcPaint.top = y;
            ps_.rcPaint.bottom = y + h;
            init(opaque);
        }

        virtual ~CanvasPaintT()
        {
            if(!isEmpty())
            {
                restoreToCount(1);
                // 提交绘图到屏幕.
                getTopPlatformDevice().drawToHDC(paint_dc_,
                    ps_.rcPaint.left, ps_.rcPaint.top, NULL);
            }
            if(for_paint_)
            {
                EndPaint(hwnd_, &ps_);
            }
        }

        // 如果无效区是空的返回true. 调用者用它来确定是否有东西需要绘制
        bool isEmpty() const
        {
            return ps_.rcPaint.right-ps_.rcPaint.left==0 ||
                ps_.rcPaint.bottom-ps_.rcPaint.top==0;
        }

        // 用于访问Windows的绘图参数, 特别是对于获取绘图矩形范围:
        // paintstruct().rcPaint.
        const PAINTSTRUCT& paintStruct() const
        {
            return ps_;
        }

        // 返回绘图DC.
        HDC paintDC() const
        {
            return paint_dc_;
        }

    protected:
        HWND hwnd_;
        HDC paint_dc_;
        PAINTSTRUCT ps_;

    private:
        void initPaint(bool opaque)
        {
            paint_dc_ = BeginPaint(hwnd_, &ps_);

            init(opaque);
        }

        void init(bool opaque)
        {
            // 修改: 对于ClearType, 我们可能需要把绘图范围扩充一个像素, 这样边界才是
            // 正确的(ClearType文本跟这个像素有关系). 只需绘制嵌入矩形的像素到屏幕.
            const int width = ps_.rcPaint.right - ps_.rcPaint.left;
            const int height = ps_.rcPaint.bottom - ps_.rcPaint.top + 1;
            if(!initialize(width, height, opaque, NULL))
            {
                // 故意引起崩溃.
                *(char*)0 = 0;
            }

            // 脏矩形的画布转换到屏幕坐标.
            translate(SkIntToScalar(-ps_.rcPaint.left),
                SkIntToScalar(-ps_.rcPaint.top));
        }

        // true表示本画布是为BeginPaint创建的.
        const bool for_paint_;

        // 禁止拷贝和赋值构造函数.
        CanvasPaintT(const CanvasPaintT&);
        CanvasPaintT& operator=(const CanvasPaintT&);
    };

    typedef CanvasPaintT<skia::PlatformCanvas> PlatformCanvasPaint;

} //namespace gfx

#endif //__gfx_canvas_paint_win_h__