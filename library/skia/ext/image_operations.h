
#ifndef __skia_ext_image_operations_h__
#define __skia_ext_image_operations_h__

#pragma once

class SkBitmap;
struct SkIRect;

namespace skia
{

    class ImageOperations
    {
    public:
        enum ResizeMethod
        {
            // Quality Methods
            //
            // Those enumeration values express a desired quality/speed tradeoff.
            // They are translated into an algorithm-specific method that depends
            // on the capabilities (CPU, GPU) of the underlying platform.
            // It is possible for all three methods to be mapped to the same
            // algorithm on a given platform.

            // Good quality resizing. Fastest resizing with acceptable visual quality.
            // This is typically intended for use during interactive layouts
            // where slower platforms may want to trade image quality for large
            // increase in resizing performance.
            //
            // For example the resizing implementation may devolve to linear
            // filtering if this enables GPU acceleration to be used.
            //
            // Note that the underlying resizing method may be determined
            // on the fly based on the parameters for a given resize call.
            // For example an implementation using a GPU-based linear filter
            // in the common case may still use a higher-quality software-based
            // filter in cases where using the GPU would actually be slower - due
            // to too much latency - or impossible - due to image format or size
            // constraints.
            RESIZE_GOOD,

            // Medium quality resizing. Close to high quality resizing (better
            // than linear interpolation) with potentially some quality being
            // traded-off for additional speed compared to RESIZE_BEST.
            //
            // This is intended, for example, for generation of large thumbnails
            // (hundreds of pixels in each dimension) from large sources, where
            // a linear filter would produce too many artifacts but where
            // a RESIZE_HIGH might be too costly time-wise.
            RESIZE_BETTER,

            // High quality resizing. The algorithm is picked to favor image quality.
            RESIZE_BEST,

            // Algorithm-specific enumerations

            // Box filter. This is a weighted average of all of the pixels touching
            // the destination pixel. For enlargement, this is nearest neighbor.
            //
            // You probably don't want this, it is here for testing since it is easy to
            // compute. Use RESIZE_LANCZOS3 instead.
            RESIZE_BOX,

            // 1-cycle Hamming filter. This is tall is the middle and falls off towards
            // the window edges but without going to 0. This is about 40% faster than
            // a 2-cycle Lanczos.
            RESIZE_HAMMING1,

            // 2-cycle Lanczos filter. This is tall in the middle, goes negative on
            // each side, then returns to zero. Does not provide as good a frequency
            // response as a 3-cycle Lanczos but is roughly 30% faster.
            RESIZE_LANCZOS2,

            // 3-cycle Lanczos filter. This is tall in the middle, goes negative on
            // each side, then oscillates 2 more times. It gives nice sharp edges.
            RESIZE_LANCZOS3,

            // Lanczos filter + subpixel interpolation. If subpixel rendering is not
            // appropriate we automatically fall back to Lanczos.
            RESIZE_SUBPIXEL,

            // enum aliases for first and last methods by algorithm or by quality.
            RESIZE_FIRST_QUALITY_METHOD = RESIZE_GOOD,
            RESIZE_LAST_QUALITY_METHOD = RESIZE_BEST,
            RESIZE_FIRST_ALGORITHM_METHOD = RESIZE_BOX,
            RESIZE_LAST_ALGORITHM_METHOD = RESIZE_SUBPIXEL,
        };

        // 使用指定的方法改变源位图的尺寸, 整个图像大小为dest_width*dest_height.
        // dest_subset是实际返回的位图部分.
        //
        // 输出位图大小为(dest_subset.width(), dest_subset.height()), 这样如果不
        // 需要整个位图时, 可以指定想要的范围.
        //
        // 返回的位图必须比修改后的位图小.
        static SkBitmap Resize(const SkBitmap& source, ResizeMethod method,
            int dest_width, int dest_height, const SkIRect& dest_subset);

        // 另外一种改变大小的方式, 返回整个位图而不是部分.
        static SkBitmap Resize(const SkBitmap& source, ResizeMethod method,
            int dest_width, int dest_height);

    private:
        ImageOperations(); // 只能使用静态成员函数.

        // 支持所有方法, 除了RESIZE_SUBPIXEL.
        static SkBitmap ResizeBasic(const SkBitmap& source, ResizeMethod method,
            int dest_width, int dest_height, const SkIRect& dest_subset);

        // Subpixel渲染器.
        static SkBitmap ResizeSubpixel(const SkBitmap& source,
            int dest_width, int dest_height, const SkIRect& dest_subset);
    };

} // namespace skia

#endif //__skia_ext_image_operations_h__