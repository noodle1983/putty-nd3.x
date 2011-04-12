
#include "native_button_win.h"

#include <commctrl.h>
#include <oleacc.h>

#include "base/win/scoped_comptr.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"

#include "../../accessibility/accessible_view_state.h"
#include "../../widget/widget.h"
#include "native_button.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // NativeButtonWin, public:

    NativeButtonWin::NativeButtonWin(NativeButton* native_button)
        : native_button_(native_button),
        button_size_valid_(false)
    {
        // Associates the actual HWND with the native_button so the native_button is
        // the one considered as having the focus (not the wrapper) when the HWND is
        // focused directly (with a click for example).
        set_focus_view(native_button);
    }

    NativeButtonWin::~NativeButtonWin() {}

    ////////////////////////////////////////////////////////////////////////////////
    // NativeButtonWin, NativeButtonWrapper implementation:

    void NativeButtonWin::UpdateLabel()
    {
        // Show or hide the shield icon of Windows onto this button every time when we
        // update the button text so Windows can lay out the shield icon and the
        // button text correctly.
        if(base::GetWinVersion()>=base::WINVERSION_VISTA &&
            base::UserAccountControlIsEnabled())
        {
#ifndef Button_SetElevationRequiredState
            // Macro to use on a button or command link to display an elevated icon
#define BCM_SETSHIELD (BCM_FIRST + 0x000C)
#define Button_SetElevationRequiredState(hwnd, fRequired) \
    (LRESULT)SNDMSG((hwnd), BCM_SETSHIELD, 0, (LPARAM)fRequired)
#endif
            Button_SetElevationRequiredState(native_view(),
                native_button_->need_elevation());
        }

        SetWindowText(native_view(), native_button_->label().c_str());
        button_size_valid_ = false;
    }

    void NativeButtonWin::UpdateFont()
    {
        SendMessage(native_view(), WM_SETFONT,
            reinterpret_cast<WPARAM>(native_button_->font().GetNativeFont()),
            FALSE);
        button_size_valid_ = false;
    }

    void NativeButtonWin::UpdateEnabled()
    {
        SetEnabled(native_button_->IsEnabled());
    }

    void NativeButtonWin::UpdateDefault()
    {
        if(!IsCheckbox())
        {
            SendMessage(native_view(), BM_SETSTYLE,
                native_button_->is_default()?BS_DEFPUSHBUTTON:BS_PUSHBUTTON,
                true);
            button_size_valid_ = false;
        }
    }

    void NativeButtonWin::UpdateAccessibleName()
    {
        AccessibleViewState state;
        native_button_->GetAccessibleState(&state);
        string16 name = state.name;
        base::ScopedComPtr<IAccPropServices> pAccPropServices;
        HRESULT hr = CoCreateInstance(CLSID_AccPropServices, NULL, CLSCTX_SERVER,
            IID_IAccPropServices, reinterpret_cast<void**>(&pAccPropServices));
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(name.c_str());
            hr = pAccPropServices->SetHwndProp(native_view(), OBJID_WINDOW,
                CHILDID_SELF, PROPID_ACC_NAME, var);
        }
    }

    View* NativeButtonWin::GetView()
    {
        return this;
    }

    void NativeButtonWin::SetFocus()
    {
        // Focus the associated HWND.
        OnFocus();
    }

    bool NativeButtonWin::UsesNativeLabel() const
    {
        return true;
    }

    bool NativeButtonWin::UsesNativeRadioButtonGroup() const
    {
        return false;
    }

    HWND NativeButtonWin::GetTestingHandle() const
    {
        return native_view();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeButtonWin, View overrides:

    gfx::Size NativeButtonWin::GetPreferredSize()
    {
        if(!button_size_valid_)
        {
            SIZE sz = { 0 };
            Button_GetIdealSize(native_view(), reinterpret_cast<LPARAM>(&sz));
            button_size_.SetSize(sz.cx, sz.cy);
            button_size_valid_ = true;
        }
        return button_size_;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeButtonWin, NativeControlWin overrides:

    bool NativeButtonWin::ProcessMessage(UINT message, WPARAM w_param,
        LPARAM l_param, LRESULT* result)
    {
        if(message==WM_COMMAND && HIWORD(w_param)==BN_CLICKED)
        {
            native_button_->ButtonPressed();
            *result = 0;
            return true;
        }
        return NativeControlWin::ProcessMessage(message, w_param, l_param, result);
    }

    bool NativeButtonWin::OnKeyDown(int vkey)
    {
        bool enter_pressed = vkey == VK_RETURN;
        if(enter_pressed)
        {
            native_button_->ButtonPressed();
        }
        return enter_pressed;
    }

    void NativeButtonWin::CreateNativeControl()
    {
        DWORD flags = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_PUSHBUTTON;
        if(native_button_->is_default())
        {
            flags |= BS_DEFPUSHBUTTON;
        }
        HWND control_hwnd = CreateWindowEx(GetAdditionalExStyle(), L"BUTTON", L"",
            flags, 0, 0, width(), height(),
            GetWidget()->GetNativeView(), NULL, NULL, NULL);
        NativeControlCreated(control_hwnd);
    }

    void NativeButtonWin::NativeControlCreated(HWND control_hwnd)
    {
        NativeControlWin::NativeControlCreated(control_hwnd);

        UpdateFont();
        UpdateLabel();
        UpdateDefault();
        UpdateAccessibleName();
    }

    // static
    NativeButtonWrapper* NativeButtonWrapper::CreateNativeButtonWrapper(
        NativeButton* native_button)
    {
        return new NativeButtonWin(native_button);
    }

} //namespace view