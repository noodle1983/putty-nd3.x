
#include "image.h"

#include "base/logging.h"
#include "base/stl_utilinl.h"

#include "SkBitmap.h"

namespace gfx
{

    namespace internal
    {

        class ImageRepSkia;

        // ImageRep存储图像的内存块. 每种RepresentationType有一个ImageRep派生类, 负责
        // 释放内存. 创建一个ImageRep后, 会接管图像的所有权, 不会维护或者增加引用计数.
        class ImageRep
        {
        public:
            explicit ImageRep(Image::RepresentationType rep) : type_(rep) {}

            // 删除ImageRep关联的像素.
            virtual ~ImageRep() {}

            // 转换辅助("假的RTTI").
            ImageRepSkia* AsImageRepSkia()
            {
                CHECK_EQ(type_, Image::kImageRepSkia);
                return reinterpret_cast<ImageRepSkia*>(this);
            }

            Image::RepresentationType type() const { return type_; }

        private:
            Image::RepresentationType type_;
        };

        class ImageRepSkia : public ImageRep
        {
        public:
            explicit ImageRepSkia(const SkBitmap* bitmap)
                : ImageRep(Image::kImageRepSkia)
            {
                CHECK(bitmap);
                bitmaps_.push_back(bitmap);
            }

            explicit ImageRepSkia(const std::vector<const SkBitmap*>& bitmaps)
                : ImageRep(Image::kImageRepSkia), bitmaps_(bitmaps)
            {
                CHECK(!bitmaps_.empty());
            }

            virtual ~ImageRepSkia()
            {
                STLDeleteElements(&bitmaps_);
            }

            const SkBitmap* bitmap() const { return bitmaps_[0]; }

            const std::vector<const SkBitmap*>& bitmaps() const { return bitmaps_; }

        private:
            std::vector<const SkBitmap*> bitmaps_;

            DISALLOW_COPY_AND_ASSIGN(ImageRepSkia);
        };

        // The Storage class acts similarly to the pixels in a SkBitmap: the Image
        // class holds a refptr instance of Storage, which in turn holds all the
        // ImageReps. This way, the Image can be cheaply copied.
        class ImageStorage : public base::RefCounted<ImageStorage>
        {
        public:
            ImageStorage(gfx::Image::RepresentationType default_type)
                : default_representation_type_(default_type) {}

            gfx::Image::RepresentationType default_representation_type()
            {
                return default_representation_type_;
            }
            gfx::Image::RepresentationMap& representations() { return representations_; }

        private:
            ~ImageStorage()
            {
                for(gfx::Image::RepresentationMap::iterator it=representations_.begin();
                    it!=representations_.end(); ++it)
                {
                    delete it->second;
                }
                representations_.clear();
            }

            // The type of image that was passed to the constructor. This key will always
            // exist in the |representations_| map.
            gfx::Image::RepresentationType default_representation_type_;

            // All the representations of an Image. Size will always be at least one, with
            // more for any converted representations.
            gfx::Image::RepresentationMap representations_;

            friend class base::RefCounted<ImageStorage>;
        };

    }


    Image::Image(const SkBitmap* bitmap)
        : storage_(new internal::ImageStorage(Image::kImageRepSkia))
    {
        internal::ImageRepSkia* rep = new internal::ImageRepSkia(bitmap);
        AddRepresentation(rep);
    }

    Image::Image(const std::vector<const SkBitmap*>& bitmaps)
        : storage_(new internal::ImageStorage(Image::kImageRepSkia))
    {
        internal::ImageRepSkia* rep = new internal::ImageRepSkia(bitmaps);
        AddRepresentation(rep);
    }

    Image::Image(const Image& other) : storage_(other.storage_) {}

    Image& Image::operator=(const Image& other)
    {
        storage_ = other.storage_;
        return *this;
    }

    Image::~Image() {}

    const SkBitmap* Image::ToSkBitmap() const
    {
        internal::ImageRep* rep = GetRepresentation(Image::kImageRepSkia);
        return rep->AsImageRepSkia()->bitmap();
    }

    const SkBitmap* Image::CopySkBitmap() const
    {
        return new SkBitmap(*ToSkBitmap());
    }

    Image::operator const SkBitmap*() const
    {
        return ToSkBitmap();
    }

    Image::operator const SkBitmap&() const
    {
        return *ToSkBitmap();
    }

    bool Image::HasRepresentation(RepresentationType type) const
    {
        return storage_->representations().count(type) != 0;
    }

    size_t Image::RepresentationCount() const
    {
        return storage_->representations().size();
    }

    void Image::SwapRepresentations(gfx::Image* other)
    {
        storage_.swap(other->storage_);
    }

    internal::ImageRep* Image::DefaultRepresentation() const
    {
        RepresentationMap& representations = storage_->representations();
        RepresentationMap::iterator it =
            representations.find(storage_->default_representation_type());
        DCHECK(it != representations.end());
        return it->second;
    }

    internal::ImageRep* Image::GetRepresentation(
        RepresentationType rep_type) const
    {
        // If the requested rep is the default, return it.
        internal::ImageRep* default_rep = DefaultRepresentation();
        if(rep_type == storage_->default_representation_type())
        {
            return default_rep;
        }

        // Check to see if the representation already exists.
        RepresentationMap::iterator it = storage_->representations().find(rep_type);
        if(it != storage_->representations().end())
        {
            return it->second;
        }

        // At this point, the requested rep does not exist, so it must be converted
        // from the default rep.

        // Handle native-to-Skia conversion.
        if(rep_type == Image::kImageRepSkia)
        {
            internal::ImageRepSkia* rep = NULL;
            CHECK(rep);
            AddRepresentation(rep);
            return rep;
        }

        // Handle Skia-to-native conversions.
        if(default_rep->type() == Image::kImageRepSkia)
        {
            internal::ImageRepSkia* skia_rep = default_rep->AsImageRepSkia();
            internal::ImageRep* native_rep = NULL;
            CHECK(native_rep);
            AddRepresentation(native_rep);
            return native_rep;
        }

        // Something went seriously wrong...
        return NULL;
    }

    void Image::AddRepresentation(internal::ImageRep* rep) const
    {
        storage_->representations().insert(std::make_pair(rep->type(), rep));
    }

    size_t Image::GetNumberOfSkBitmaps() const 
    {
        return GetRepresentation(Image::kImageRepSkia)->AsImageRepSkia()->
            bitmaps().size();
    }

    const SkBitmap* Image::GetSkBitmapAtIndex(size_t index) const
    {
        return GetRepresentation(Image::kImageRepSkia)->AsImageRepSkia()->
            bitmaps()[index];
    }

} //namespace gfx