
#ifndef __gfx_image_h__
#define __gfx_image_h__

#pragma once

#include <map>

#include "base/basic_types.h"

class SkBitmap;

namespace gfx
{

    class ImageRep;

    // 所有图像类型的封装类, 可以是本地平台的GdkBitmap/NSImage或者SkBitmap.
    // 通过操作符重载可以转换到其它图像类型. 内部会缓存转换过的图像避免重复转换.
    //
    // 初始的图像对象以及转换的图像对象生命周期和Image对象一致.
    class Image
    {
    public:
        enum RepresentationType
        {
            kGdkPixbufRep,
            kNSImageRep,
            kSkBitmapRep,
        };

        // 创建缺省图像类型的Image. 对象接管bitmap所有权.
        explicit Image(const SkBitmap* bitmap);

        // 删除图像以及内部缓存的转换图像.
        ~Image();

        // 图像类型转换处理.
        operator const SkBitmap*();
        operator const SkBitmap&();

        // 查看是否有指定类型的图像.
        bool HasRepresentation(RepresentationType type);

        // 和|other|交互内部存储的图像.
        void SwapRepresentations(gfx::Image* other);

    private:
        // 返回缺省的ImageRep.
        ImageRep* DefaultRepresentation();

        // 返回指定类型的ImageRep, 必要时转换并缓存.
        ImageRep* GetRepresentation(RepresentationType rep);

        // 存储到map.
        void AddRepresentation(ImageRep* rep);

        // 构造函数传递的图像类型. 这个键总是存在于|representations_|中.
        RepresentationType default_representation_;

        typedef std::map<RepresentationType, ImageRep*> RepresentationMap;
        // 所有图像类型. 至少有一个, 最多每种类型有一个转换对象.
        RepresentationMap representations_;

        DISALLOW_COPY_AND_ASSIGN(Image);
    };

} //namespace gfx

#endif //__gfx_image_h__