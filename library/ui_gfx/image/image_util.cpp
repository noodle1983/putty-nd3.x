
#include "image_util.h"

#include "base/memory/scoped_ptr.h"

#include "SkBitmap.h"

#include "ui_gfx/codec/png_codec.h"
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

} //namespace gfx