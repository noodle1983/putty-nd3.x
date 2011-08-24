
#ifndef __ui_gfx_font_h__
#define __ui_gfx_font_h__

#pragma once

#include "base/memory/ref_counted.h"
#include "base/string16.h"

namespace gfx
{

    class PlatformFont;

    // Font提供底层字体的封装. 允许拷贝和赋值操作, 且非常低廉(引用计数).
    class Font
    {
    public:
        // 字体风格.
        enum FontStyle
        {
            NORMAL = 0,
            BOLD = 1,
            ITALIC = 2,
            UNDERLINED = 4,
        };

        // 创建缺省名字和风格的字体.
        Font();

        // 克隆字体对象.
        Font(const Font& other);
        Font& operator=(const Font& other);

        // 从本地字体创建Font对象.
        explicit Font(HFONT native_font);

        // 用PlatformFont对象构建Font. Font对象接管PlatformFont对象的所有权.
        explicit Font(PlatformFont* platform_font);

        // 用指定名字和字号创建字体.
        Font(const string16& font_name, int font_size);

        ~Font();

        // 从存在的字体返回一个新的字体, 字号增加size_delta. 比如, 5返回的字体
        // 比这个字体大5个单位.
        Font DeriveFont(int size_delta) const;

        // 从存在的字体返回一个新的字体, 字号增加size_delta. 比如, 5返回的字体
        // 比这个字体大5个单位.
        // style参数指定新的字体风格, 是这几个值的组合: BOLD、ITALIC和UNDERLINED.
        Font DeriveFont(int size_delta, int style) const;

        // 返回显示字符所需的垂直像素. 包含行距, 就是说高度可能比ascent+descent大.
        int GetHeight() const;

        // 返回字体的baseline或者ascent.
        int GetBaseline() const;

        // 返回字体的平均字符宽度.
        int GetAverageCharacterWidth() const;

        // 返回显示文本所需的水平像素.
        int GetStringWidth(const string16& text) const;

        // 返回显示指定长度字符所需的水平像素. 调用GetStringWidth()获取实际数目.
        int GetExpectedTextWidth(int length) const;

        // 返回字体风格.
        int GetStyle() const;

        // 返回字体名字.
        string16 GetFontName() const;

        // 返回字号(像素).
        int GetFontSize() const;

        // 返回本地字体句柄.
        HFONT GetNativeFont() const;

        // 获取底层平台实现. 可以根据需要强转到具体实现类型.
        PlatformFont* platform_font() const { return platform_font_.get(); }

    private:
        // 平台字体封装实现.
        scoped_refptr<PlatformFont> platform_font_;
    };

} //namespace gfx

#endif //__ui_gfx_font_h__