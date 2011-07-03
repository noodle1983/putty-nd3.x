
#include "native_listbox_win.h"

#include <commctrl.h>
#include <windowsx.h>

#include "base/utf_string_conversions.h"

#include "ui_gfx/font.h"
#include "ui_gfx/native_theme_win.h"

#include "ui_base/l10n/l10n_util_win.h"
#include "ui_base/resource/resource_bundle.h"

#include "listbox.h"
#include "listbox_model.h"
#include "view/widget/widget.h"

namespace view
{

    // Limit how small a listbox can be.
    static const int kMinListboxWidth = 148;

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWin, public:

    NativeListboxWin::NativeListboxWin(Listbox* listbox)
        : listbox_(listbox), content_width_(0)
    {
        // Associates the actual HWND with the listbox so the listbox is the one
        // considered as having the focus (not the wrapper) when the HWND is
        // focused directly (with a click for example).
        set_focus_view(listbox);
    }

    NativeListboxWin::~NativeListboxWin() {}

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWin, NativeListboxWrapper implementation:

    void NativeListboxWin::UpdateFromModel()
    {
        SendMessage(native_view(), LB_RESETCONTENT, 0, 0);
        gfx::Font font = ui::ResourceBundle::GetSharedInstance().GetFont(
            ui::ResourceBundle::BaseFont);
        int max_width = 0;
        int num_items = listbox_->model()->GetItemCount();
        for(int i=0; i<num_items; ++i)
        {
            std::wstring text = UTF16ToWide(listbox_->model()->GetItemAt(i));

            // Inserting the Unicode formatting characters if necessary so that the
            // text is displayed correctly in right-to-left UIs.
            base::i18n::AdjustStringForLocaleDirection(&text);
            const wchar_t* text_ptr = text.c_str();

            ListBox_AddString(native_view(), text_ptr);
            max_width = std::max(max_width, font.GetStringWidth(text));
        }
        content_width_ = max_width;
    }

    void NativeListboxWin::UpdateEnabled()
    {
        SetEnabled(listbox_->IsEnabled());
    }

    int NativeListboxWin::GetRowCount() const
    {
        if(!native_view())
        {
            return 0;
        }
        return ListBox_GetCount(native_view());
    }

    int NativeListboxWin::SelectedRow() const
    {
        if(!native_view())
        {
            return -1;
        }
        return ListBox_GetCurSel(native_view());
    }

    void NativeListboxWin::SelectRow(int row)
    {
        if(!native_view())
        {
            return;
        }
        ListBox_SetCurSel(native_view(), row);
    }

    gfx::Size NativeListboxWin::GetPreferredSize()
    {
        gfx::Size border(GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));

        // The cx computation can be read as measuring from left to right.
        int pref_width = std::max(content_width_+2*border.width(), kMinListboxWidth);

        // 高度计算为8行.
        gfx::Font font = ui::ResourceBundle::GetSharedInstance().GetFont(
            ui::ResourceBundle::BaseFont);
        int pref_height = font.GetHeight()*8 + 2*border.height();
        return gfx::Size(pref_width, pref_height);
    }

    View* NativeListboxWin::GetView()
    {
        return this;
    }

    void NativeListboxWin::SetFocus()
    {
        OnFocus();
    }

    HWND NativeListboxWin::GetTestingHandle() const
    {
        return native_view();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWin, NativeControlWin overrides:

    bool NativeListboxWin::ProcessMessage(UINT message, WPARAM w_param,
        LPARAM l_param, LRESULT* result)
    {
        if(message==WM_COMMAND && HIWORD(w_param)==LBN_SELCHANGE)
        {
            listbox_->SelectionChanged();
            *result = 0;
            return true;
        }

        return NativeControlWin::ProcessMessage(message, w_param, l_param, result);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWin, protected:

    void NativeListboxWin::CreateNativeControl()
    {
        int style = WS_CHILD | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            LBS_NOINTEGRALHEIGHT | LBS_NOTIFY;
        // If there's only one column and the title string is empty, don't show a
        // header.
        HWND control_hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE|GetAdditionalRTLStyle(),
            WC_LISTBOX,
            L"",
            style,
            0, 0, width(), height(),
            listbox_->GetWidget()->GetNativeView(),
            NULL, NULL, NULL);
        NativeControlCreated(control_hwnd);
    }

    void NativeListboxWin::NativeControlCreated(HWND native_control)
    {
        NativeControlWin::NativeControlCreated(native_control);

        UpdateFont();
        UpdateFromModel();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWin, private:

    void NativeListboxWin::UpdateFont()
    {
        HFONT font = ui::ResourceBundle::GetSharedInstance().
            GetFont(ui::ResourceBundle::BaseFont).GetNativeFont();
        SendMessage(native_view(), WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
        ui::AdjustUIFontForWindow(native_view());
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeListboxWrapper, public:

    // static
    NativeListboxWrapper* NativeListboxWrapper::CreateNativeWrapper(Listbox* listbox)
    {
        return new NativeListboxWin(listbox);
    }

} //namespace view