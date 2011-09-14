
#include "menu_config.h"

#include <uxtheme.h>
#include <vssym32.h>

#include "base/logging.h"
#include "base/win/win_util.h"

#include "ui_gfx/native_theme_win.h"

#include "ui_base/l10n/l10n_util_win.h"

using gfx::NativeTheme;
using gfx::NativeThemeWin;

namespace view
{

    static MenuConfig* config_instance = NULL;

    MenuConfig::MenuConfig()
        : text_color(SK_ColorBLACK),
        item_top_margin(3),
        item_bottom_margin(4),
        item_no_icon_top_margin(1),
        item_no_icon_bottom_margin(3),
        item_left_margin(4),
        label_to_arrow_padding(10),
        arrow_to_edge_padding(5),
        icon_to_label_padding(8),
        gutter_to_label(5),
        check_width(16),
        check_height(16),
        radio_width(16),
        radio_height(16),
        arrow_height(9),
        arrow_width(9),
        gutter_width(0),
        separator_height(6),
        render_gutter(false),
        show_mnemonics(false),
        scroll_arrow_height(3),
        label_to_accelerator_padding(10),
        show_accelerators(true) {}

    MenuConfig::~MenuConfig() {}

    void MenuConfig::Reset()
    {
        delete config_instance;
        config_instance = NULL;
    }

    // static
    const MenuConfig& MenuConfig::instance()
    {
        if(!config_instance)
        {
            config_instance = Create();
        }
        return *config_instance;
    }

    // static
    MenuConfig* MenuConfig::Create()
    {
        MenuConfig* config = new MenuConfig();

        config->text_color = NativeThemeWin::instance()->GetThemeColorWithDefault(
            NativeThemeWin::MENU, MENU_POPUPITEM, MPI_NORMAL, TMT_TEXTCOLOR,
            COLOR_MENUTEXT);

        NONCLIENTMETRICS metrics;
        base::win::GetNonClientMetrics(&metrics);
        ui::AdjustUIFont(&(metrics.lfMenuFont));
        HFONT font = CreateFontIndirect(&metrics.lfMenuFont);
        DLOG_ASSERT(font);
        config->font = gfx::Font(font);

        NativeTheme::ExtraParams extra;
        extra.menu_check.is_radio = false;
        gfx::Size check_size = NativeTheme::instance()->GetPartSize(
            NativeTheme::kMenuCheck, NativeTheme::kNormal, extra);
        if(!check_size.IsEmpty())
        {
            config->check_width = check_size.width();
            config->check_height = check_size.height();
        }
        else
        {
            config->check_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->check_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        extra.menu_check.is_radio = true;
        gfx::Size radio_size = NativeTheme::instance()->GetPartSize(
            NativeTheme::kMenuCheck, NativeTheme::kNormal, extra);
        if(!radio_size.IsEmpty())
        {
            config->radio_width = radio_size.width();
            config->radio_height = radio_size.height();
        }
        else
        {
            config->radio_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->radio_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        gfx::Size arrow_size = NativeTheme::instance()->GetPartSize(
            NativeTheme::kMenuPopupArrow, NativeTheme::kNormal, extra);
        if(!arrow_size.IsEmpty())
        {
            config->arrow_width = arrow_size.width();
            config->arrow_height = arrow_size.height();
        }
        else
        {
            // Sadly I didn't see a specify metrics for this.
            config->arrow_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->arrow_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        gfx::Size gutter_size = NativeTheme::instance()->GetPartSize(
            NativeTheme::kMenuPopupGutter, NativeTheme::kNormal, extra);
        if(!gutter_size.IsEmpty())
        {
            config->gutter_width = gutter_size.width();
            config->render_gutter = true;
        }
        else
        {
            config->gutter_width = 0;
            config->render_gutter = false;
        }

        gfx::Size separator_size = NativeTheme::instance()->GetPartSize(
            NativeTheme::kMenuPopupSeparator, NativeTheme::kNormal, extra);
        if(!separator_size.IsEmpty())
        {
            config->separator_height = separator_size.height();
        }
        else
        {
            // -1 makes separator centered.
            config->separator_height = GetSystemMetrics(SM_CYMENU) / 2 - 1;
        }

        // On Windows, having some menus use wider spacing than others looks wrong.
        // See http://crbug.com/88875
        config->item_no_icon_bottom_margin = config->item_bottom_margin;
        config->item_no_icon_top_margin = config->item_top_margin;

        BOOL show_cues;
        config->show_mnemonics = (SystemParametersInfo(SPI_GETKEYBOARDCUES, 0,
            &show_cues, 0) && show_cues==TRUE);
        return config;
    }

} //namespace view