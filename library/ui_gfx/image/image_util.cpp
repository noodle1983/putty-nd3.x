
#include "image_util.h"

#include "base/memory/scoped_ptr.h"

#include "SkBitmap.h"

#include "ui_gfx/codec/png_codec.h"
#include "ui_gfx/codec/jpeg_codec.h"
#include "ui_gfx/image/image.h"

namespace gfx
{

    Image* ImageFromPNGEncodedData(const unsigned char* input, size_t input_size)
    {
        scoped_ptr<SkBitmap> favicon_bitmap(new SkBitmap());
        if(gfx::PNGCodec::Decode(input, input_size, favicon_bitmap.get()))
        {
            return new Image(favicon_bitmap.release());
        }

        return NULL;
    }

    bool PNGEncodedDataFromImage(const Image& image,
        std::vector<unsigned char>* dst)
    {
        const SkBitmap& bitmap = image;
        return gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, dst);
    }

    bool JPEGEncodedDataFromImage(const Image& image,
        std::vector<unsigned char>* dst)
    {
        const SkBitmap& bitmap = image;
        SkAutoLockPixels bitmap_lock(bitmap);

        if(!bitmap.readyToDraw())
        {
            return false;
        }

        // Use 90 quality (out of 100) which is pretty high, because
        // we're very sensitive to artifacts for these small sized,
        // highly detailed images.
        return gfx::JPEGCodec::Encode(
            reinterpret_cast<unsigned char*>(bitmap.getAddr32(0, 0)),
            gfx::JPEGCodec::FORMAT_BGRA, bitmap.width(),
            bitmap.height(),
            static_cast<int>(bitmap.rowBytes()), 90,
            dst);
    }

} //namespace gfx