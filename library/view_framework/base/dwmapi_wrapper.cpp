
#include "dwmapi_wrapper.h"

namespace view
{

    /* static */
    const DwmapiWrapper* DwmapiWrapper::GetInstance()
    {
        // È«¾ÖÊµÀý.
        static const DwmapiWrapper s_dwmapi;
        return &s_dwmapi;
    }

    DwmapiWrapper::DwmapiWrapper()
        : dwmapi_dll_(LoadLibrary(L"dwmapi.dll")),
        dwm_def_window_proc_(NULL),
        dwm_extend_frame_into_client_area_(NULL),
        dwm_is_composition_enabled_(NULL),
        dwm_set_window_attribute_(NULL)
    {
        if(dwmapi_dll_)
        {
            dwm_def_window_proc_ =
                reinterpret_cast<DwmDefWindowProcPtr>(
                GetProcAddress(dwmapi_dll_, "DwmDefWindowProc"));
            dwm_extend_frame_into_client_area_ =
                reinterpret_cast<DwmExtendFrameIntoClientAreaPtr>(
                GetProcAddress(dwmapi_dll_, "DwmExtendFrameIntoClientArea"));
            dwm_is_composition_enabled_ =
                reinterpret_cast<DwmIsCompositionEnabledPtr>(
                GetProcAddress(dwmapi_dll_, "DwmIsCompositionEnabled"));
            dwm_set_window_attribute_ =
                reinterpret_cast<DwmSetWindowAttributePtr>(
                GetProcAddress(dwmapi_dll_, "DwmSetWindowAttribute"));
        }
    }

    DwmapiWrapper::~DwmapiWrapper()
    {
        if(dwmapi_dll_)
        {
            FreeLibrary(dwmapi_dll_);
        }
    }

    BOOL DwmapiWrapper::DwmDefWindowProc(HWND hWnd, UINT msg,
        WPARAM wParam, LPARAM lParam, LRESULT* plResult) const
    {
        if(dwm_def_window_proc_)
        {
            return dwm_def_window_proc_(hWnd, msg, wParam,
                lParam, plResult);
        }

        return FALSE;
    }

    HRESULT DwmapiWrapper::DwmExtendFrameIntoClientArea(HWND hWnd,
        const MARGINS* pMarInset) const
    {
        if(dwm_extend_frame_into_client_area_)
        {
            return dwm_extend_frame_into_client_area_(hWnd, pMarInset);
        }

        return E_NOTIMPL;
    }

    HRESULT DwmapiWrapper::DwmIsCompositionEnabled(BOOL* pfEnabled) const
    {
        if(dwm_is_composition_enabled_)
        {
            return dwm_is_composition_enabled_(pfEnabled);
        }

        return E_NOTIMPL;
    }

    HRESULT DwmapiWrapper::DwmSetWindowAttribute(HWND hwnd,
        DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) const
    {
        if(dwm_set_window_attribute_)
        {
            return dwm_set_window_attribute_(hwnd, dwAttribute,
                pvAttribute, cbAttribute);
        }

        return E_NOTIMPL;
    }

} //namespace view