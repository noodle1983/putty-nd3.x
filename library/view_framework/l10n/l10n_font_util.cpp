
#include "l10n_font_util.h"

#include "base/logging.h"
#include "base/string_number_conversions.h"

#include "gfx/font.h"

#include "l10n_util.h"

namespace view
{

    int GetLocalizedContentsWidthForFont(int col_resource_id,
        const gfx::Font& font)
    {
        double chars = 0;
        base::StringToDouble(GetStringUTF8(col_resource_id), &chars);
        int width = font.GetExpectedTextWidth(static_cast<int>(chars));
        DCHECK_GT(width, 0);
        return width;
    }

    int GetLocalizedContentsHeightForFont(int row_resource_id,
        const gfx::Font& font)
    {
        double lines = 0;
        base::StringToDouble(GetStringUTF8(row_resource_id), &lines);
        int height = static_cast<int>(font.GetHeight() * lines);
        DCHECK_GT(height, 0);
        return height;
    }

    gfx::Size GetLocalizedContentsSizeForFont(int col_resource_id,
        int row_resource_id, const gfx::Font& font)
    {
        return gfx::Size(GetLocalizedContentsWidthForFont(col_resource_id, font),
            GetLocalizedContentsHeightForFont(row_resource_id, font));
    }

} //namespace view