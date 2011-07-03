
#include "gdi_util.h"

namespace gfx
{

    void CreateBitmapHeader(int width, int height, BITMAPINFOHEADER* hdr)
    {
        CreateBitmapHeaderWithColorDepth(width, height, 32, hdr);
    }

    void CreateBitmapHeaderWithColorDepth(int width, int height,
        int color_depth, BITMAPINFOHEADER* hdr)
    {
        hdr->biSize = sizeof(BITMAPINFOHEADER);
        hdr->biWidth = width;
        hdr->biHeight = -height; // 负号表示自上而下的位图.
        hdr->biPlanes = 1;
        hdr->biBitCount = color_depth;
        hdr->biCompression = BI_RGB; // 不压缩.
        hdr->biSizeImage = 0;
        hdr->biXPelsPerMeter = 1;
        hdr->biYPelsPerMeter = 1;
        hdr->biClrUsed = 0;
        hdr->biClrImportant = 0;
    }

    void CreateBitmapV4Header(int width, int height, BITMAPV4HEADER* hdr)
    {
        // 因为bmp v4表头只是一个扩展, 所以我们只创建一个v3表头, 拷贝位到v4表头中.
        BITMAPINFOHEADER header_v3;
        CreateBitmapHeader(width, height, &header_v3);
        memset(hdr, 0, sizeof(BITMAPV4HEADER));
        memcpy(hdr, &header_v3, sizeof(BITMAPINFOHEADER));

        // Correct the size of the header and fill in the mask values.
        // 修正表头大小并填充掩码值.
        hdr->bV4Size = sizeof(BITMAPV4HEADER);
        hdr->bV4RedMask   = 0x00ff0000;
        hdr->bV4GreenMask = 0x0000ff00;
        hdr->bV4BlueMask  = 0x000000ff;
        hdr->bV4AlphaMask = 0xff000000;
    }

    // 创建一个黑白的BITMAPINFOHEADER.
    void CreateMonochromeBitmapHeader(int width,
        int height, BITMAPINFOHEADER* hdr)
    {
        hdr->biSize = sizeof(BITMAPINFOHEADER);
        hdr->biWidth = width;
        hdr->biHeight = -height;
        hdr->biPlanes = 1;
        hdr->biBitCount = 1;
        hdr->biCompression = BI_RGB;
        hdr->biSizeImage = 0;
        hdr->biXPelsPerMeter = 1;
        hdr->biYPelsPerMeter = 1;
        hdr->biClrUsed = 0;
        hdr->biClrImportant = 0;
    }

    void SubtractRectanglesFromRegion(HRGN hrgn,
        const std::vector<gfx::Rect>& cutouts)
    {
        if(cutouts.size())
        {
            HRGN cutout = ::CreateRectRgn(0, 0, 0, 0);
            for(size_t i=0; i<cutouts.size(); i++)
            {
                ::SetRectRgn(cutout,
                    cutouts[i].x(),
                    cutouts[i].y(),
                    cutouts[i].right(),
                    cutouts[i].bottom());
                ::CombineRgn(hrgn, hrgn, cutout, RGN_DIFF);
            }
            ::DeleteObject(cutout);
        }
    }

} //namespace gfx