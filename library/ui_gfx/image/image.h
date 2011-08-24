
#ifndef __ui_gfx_image_h__
#define __ui_gfx_image_h__

#pragma once

#include <map>
#include <vector>

#include "base/memory/ref_counted.h"

class SkBitmap;

namespace gfx
{

    namespace internal
    {
        class ImageRep;
        class ImageStorage;
    }

    // 所有图像类型的封装类, 可以是本地平台的GdkBitmap/NSImage或者SkBitmap.
    // 通过操作符重载可以转换到其它图像类型. 内部会缓存转换过的图像避免重复转换.
    //
    // 初始的图像对象以及转换的图像对象生命周期和Image对象一致.
    class Image
    {
    public:
        enum RepresentationType
        {
            kImageRepGdk,
            kImageRepCocoa,
            kImageRepSkia,
        };

        typedef std::map<RepresentationType, internal::ImageRep*> RepresentationMap;

        // 创建缺省图像类型的Image. 对象接管bitmap所有权.
        explicit Image(const SkBitmap* bitmap);

        // To create an Image that supports multiple resolutions pass a vector
        // of bitmaps, one for each resolution.
        explicit Image(const std::vector<const SkBitmap*>& bitmaps);

        // Initializes a new Image by AddRef()ing |other|'s internal storage.
        Image(const Image& other);

        // Copies a reference to |other|'s storage.
        Image& operator=(const Image& other);

        // 删除图像以及内部缓存的转换图像.
        ~Image();

        // Converts the Image to the desired representation and stores it internally.
        // The returned result is a weak pointer owned by and scoped to the life of
        // the Image.
        const SkBitmap* ToSkBitmap() const;

        // Performs a conversion, like above, but returns a copy of the result rather
        // than a weak pointer. The caller is responsible for deleting the result.
        // Note that the result is only a copy in terms of memory management; the
        // backing pixels are shared amongst all copies (a fact of each of the
        // converted representations, rather than a limitation imposed by Image) and
        // so the result should be considered immutable.
        const SkBitmap* CopySkBitmap() const;

        // 图像类型转换处理.
        operator const SkBitmap*() const;
        operator const SkBitmap&() const;

        // Gets the number of bitmaps in this image. This may cause a conversion
        // to a bitmap representation. Note, this function and GetSkBitmapAtIndex()
        // are primarily meant to be used by the theme provider.
        size_t GetNumberOfSkBitmaps() const;

        // Gets the bitmap at the given index. This may cause a conversion
        // to a bitmap representation. Note, the internal ordering of bitmaps is not
        // guaranteed.
        const SkBitmap* GetSkBitmapAtIndex(size_t index) const;

        // 查看是否有指定类型的图像.
        bool HasRepresentation(RepresentationType type) const;

        // Returns the number of representations.
        size_t RepresentationCount() const;

        // 和|other|交互内部存储的图像.
        void SwapRepresentations(gfx::Image* other);

    private:
        // 返回缺省的ImageRep.
        internal::ImageRep* DefaultRepresentation() const;

        // 返回指定类型的ImageRep, 必要时转换并缓存.
        internal::ImageRep* GetRepresentation(RepresentationType rep) const;

        // 存储到map.
        void AddRepresentation(internal::ImageRep* rep) const;

        // Internal class that holds all the representations. This allows the Image to
        // be cheaply copied.
        scoped_refptr<internal::ImageStorage> storage_;
    };

} //namespace gfx

#endif //__ui_gfx_image_h__