
#include "menu_config.h"

#include <uxtheme.h>
#include <vssym32.h>

#include "base/logging.h"
#include "base/win/win_util.h"

#include "gfx/native_theme_win.h"

#include "../../l10n/l10n_util_win.h"

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
        label_to_accelerator_padding(10) {}

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
        base::GetNonClientMetrics(&metrics);
        AdjustUIFont(&(metrics.lfMenuFont));
        HFONT font = CreateFontIndirect(&metrics.lfMenuFont);
        DLOG_ASSERT(font);
        config->font = gfx::Font(font);

        HDC dc = GetDC(NULL);
        RECT bounds = { 0, 0, 200, 200 };
        SIZE check_size;
        if(NativeThemeWin::instance()->GetThemePartSize(NativeThemeWin::MENU,
            dc, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, &bounds,
            TS_TRUE, &check_size) == S_OK)
        {
            config->check_width = check_size.cx;
            config->check_height = check_size.cy;
        }
        else
        {
            config->check_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->check_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        SIZE radio_size;
        if(NativeThemeWin::instance()->GetThemePartSize(NativeThemeWin::MENU,
            dc, MENU_POPUPCHECK, MC_BULLETNORMAL, &bounds, TS_TRUE,
            &radio_size) == S_OK)
        {
            config->radio_width = radio_size.cx;
            config->radio_height = radio_size.cy;
        }
        else
        {
            config->radio_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->radio_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        SIZE arrow_size;
        if(NativeThemeWin::instance()->GetThemePartSize(NativeThemeWin::MENU,
            dc, MENU_POPUPSUBMENU, MSM_NORMAL, &bounds, TS_TRUE,
            &arrow_size) == S_OK)
        {
            config->arrow_width = arrow_size.cx;
            config->arrow_height = arrow_size.cy;
        }
        else
        {
            // Sadly I didn't see a specify metrics for this.
            config->arrow_width = GetSystemMetrics(SM_CXMENUCHECK);
            config->arrow_height = GetSystemMetrics(SM_CYMENUCHECK);
        }

        SIZE gutter_size;
        if(NativeThemeWin::instance()->GetThemePartSize(NativeThemeWin::MENU,
            dc, MENU_POPUPGUTTER, MSM_NORMAL, &bounds, TS_TRUE,
            &gutter_size) == S_OK)
        {
            config->gutter_width = gutter_size.cx;
            config->render_gutter = true;
        }
        else
        {
            config->gutter_width = 0;
            config->render_gutter = false;
        }

        SIZE separator_size;
        if(NativeThemeWin::instance()->GetThemePartSize(NativeThemeWin::MENU,
            dc, MENU_POPUPSEPARATOR, MSM_NORMAL, &bounds, TS_TRUE,
            &separator_size) == S_OK)
        {
            config->separator_height = separator_size.cy;
        }
        else
        {
            // -1 makes separator centered.
            config->separator_height = GetSystemMetrics(SM_CYMENU) / 2 - 1;
        }

        ReleaseDC(NULL, dc);

        BOOL show_cues;
        config->show_mnemonics = (SystemParametersInfo(SPI_GETKEYBOARDCUES, 0,
            &show_cues, 0) && show_cues==TRUE);
        return config;
    }

} //namespace view