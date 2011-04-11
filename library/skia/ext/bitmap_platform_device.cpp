
#include "bitmap_platform_device_win.h"

#include "SkUtils.h"

#include "bitmap_platform_device_data.h"

namespace
{

    // 约束position和size, 使之限定在[0, available_size]中.
    // 如果|size|是-1, 填满available_size. 如果position超出available_size,
    // 函数返回false.
    bool Constrain(int available_size, int* position, int* size)
    {
        if(*size < -2)
        {
            return false;
        }

        // 规整到原点.
        if(*position < 0)
        {
            if(*size != -1)
            {
                *size += *position;
            }
            *position = 0;
        }
        if(*size==0 || *position>=available_size)
        {
            return false;
        }

        if(*size > 0)
        {
            int overflow = (*position + *size) - available_size;
            if(overflow > 0)
            {
                *size -= overflow;
            }
        }
        else
        {
            // 填满available_size.
            *size = available_size - *position;
        }
        return true;
    }

}

namespace skia
{

    void BitmapPlatformDevice::makeOpaque(int x, int y, int width, int height)
    {
        const SkBitmap& bitmap = accessBitmap(true);
        SkASSERT(bitmap.config() == SkBitmap::kARGB_8888_Config);

        // 修改: 这种做法不太好, 不应该在这个层面处理变换. PlatformCanvas应该提供
        // 处理变换的函数(使用变换而不是变通), 传递给我们的是已经变换过的矩形.
        const SkMatrix& matrix = data_->transform();
        int bitmap_start_x = SkScalarRound(matrix.getTranslateX()) + x;
        int bitmap_start_y = SkScalarRound(matrix.getTranslateY()) + y;

        if(Constrain(bitmap.width(), &bitmap_start_x, &width) &&
            Constrain(bitmap.height(), &bitmap_start_y, &height))
        {
            SkAutoLockPixels lock(bitmap);
            SkASSERT(bitmap.rowBytes()%sizeof(uint32_t) == 0u);
            size_t row_words = bitmap.rowBytes() / sizeof(uint32_t);
            // 指针指向第一个修改的像素.
            uint32_t* data = bitmap.getAddr32(0, 0) + (bitmap_start_y * row_words) +
                bitmap_start_x;
            for(int i=0; i<height; i++)
            {
                for(int j=0; j<width; j++)
                {
                    data[j] |= (0xFF << SK_A32_SHIFT);
                }
                data += row_words;
            }
        }
    }

} //namespace skia