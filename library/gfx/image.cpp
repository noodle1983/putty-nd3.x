
#include "image.h"

#include "base/logging.h"

#include "SkBitmap.h"

namespace gfx
{

    class SkBitmapRep;

    // ImageRep存储图像的内存块. 每种RepresentationType有一个ImageRep派生类, 负责
    // 释放内存. 创建一个ImageRep后, 会接管图像的所有权, 不会维护或者增加引用计数.
    class ImageRep
    {
    public:
        explicit ImageRep(Image::RepresentationType rep) : type_(rep) {}

        // 删除ImageRep关联的像素.
        virtual ~ImageRep() {}

        // 转换辅助("假的RTTI").
        SkBitmapRep* AsSkBitmapRep()
        {
            CHECK_EQ(type_, Image::kSkBitmapRep);
            return reinterpret_cast<SkBitmapRep*>(this);
        }

        Image::RepresentationType type() const { return type_; }

    private:
        Image::RepresentationType type_;
    };

    class SkBitmapRep : public ImageRep
    {
    public:
        explicit SkBitmapRep(const SkBitmap* bitmap)
            : ImageRep(Image::kSkBitmapRep),
            bitmap_(bitmap)
        {
            CHECK(bitmap);
        }

        virtual ~SkBitmapRep()
        {
            delete bitmap_;
            bitmap_ = NULL;
        }

        const SkBitmap* bitmap() const { return bitmap_; }

    private:
        const SkBitmap* bitmap_;

        DISALLOW_COPY_AND_ASSIGN(SkBitmapRep);
    };


    Image::Image(const SkBitmap* bitmap)
        : default_representation_(Image::kSkBitmapRep)
    {
        SkBitmapRep* rep = new SkBitmapRep(bitmap);
        AddRepresentation(rep);
    }

    Image::~Image()
    {
        for(RepresentationMap::iterator it=representations_.begin();
            it!=representations_.end(); ++it)
        {
            delete it->second;
        }
        representations_.clear();
    }

    Image::operator const SkBitmap*()
    {
        ImageRep* rep = GetRepresentation(Image::kSkBitmapRep);
        return rep->AsSkBitmapRep()->bitmap();
    }

    Image::operator const SkBitmap&()
    {
        return *(this->operator const SkBitmap*());
    }

    bool Image::HasRepresentation(RepresentationType type)
    {
        return representations_.count(type) != 0;
    }

    void Image::SwapRepresentations(gfx::Image* other)
    {
        representations_.swap(other->representations_);
        std::swap(default_representation_, other->default_representation_);
    }

    ImageRep* Image::DefaultRepresentation()
    {
        RepresentationMap::iterator it =
            representations_.find(default_representation_);
        DCHECK(it != representations_.end());
        return it->second;
    }

    ImageRep* Image::GetRepresentation(RepresentationType rep_type)
    {
        // 如果请求缺省的, 直接返回.
        ImageRep* default_rep = DefaultRepresentation();
        if(rep_type == default_representation_)
        {
            return default_rep;
        }

        // 检查是否已经存在.
        RepresentationMap::iterator it = representations_.find(rep_type);
        if(it != representations_.end())
        {
            return it->second;
        }

        // 此时, 需要转换的类型不存在, 必须从缺省的类型转换.

        // 处理本地到Skia的转换.
        if(rep_type == Image::kSkBitmapRep)
        {
            SkBitmapRep* rep = NULL;
            if(rep)
            {
                AddRepresentation(rep);
                return rep;
            }
            NOTREACHED();
        }

        // 处理Skia到本地的转换.
        if(default_rep->type() == Image::kSkBitmapRep)
        {
            SkBitmapRep* skia_rep = default_rep->AsSkBitmapRep();
            ImageRep* native_rep = NULL;
            if(native_rep)
            {
                AddRepresentation(native_rep);
                return native_rep;
            }
            NOTREACHED();
        }

        // 发生了严重的错误...
        return NULL;
    }

    void Image::AddRepresentation(ImageRep* rep)
    {
        representations_.insert(std::make_pair(rep->type(), rep));
    }

} //namespace gfx