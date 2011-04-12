
#ifndef __view_native_button_win_h__
#define __view_native_button_win_h__

#include "../native_control_win.h"
#include "native_button_wrapper.h"

namespace view
{

    // A View that hosts a native Windows button.
    class NativeButtonWin : public NativeControlWin,
        public NativeButtonWrapper
    {
    public:
        explicit NativeButtonWin(NativeButton* native_button);
        virtual ~NativeButtonWin();

        // Overridden from NativeButtonWrapper:
        virtual void UpdateLabel();
        virtual void UpdateFont();
        virtual void UpdateEnabled();
        virtual void UpdateDefault();
        virtual void UpdateAccessibleName();
        virtual View* GetView();
        virtual void SetFocus();
        virtual bool UsesNativeLabel() const;
        virtual bool UsesNativeRadioButtonGroup() const;
        virtual HWND GetTestingHandle() const;

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();

        // Overridden from NativeControlWin:
        virtual bool ProcessMessage(UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT* result);
        virtual bool OnKeyDown(int vkey);

    protected:
        virtual void CreateNativeControl();
        virtual void NativeControlCreated(HWND control_hwnd);

        // Returns true if this button is actually a checkbox or radio button.
        virtual bool IsCheckbox() const { return false; }

    private:
        // The NativeButton we are bound to.
        NativeButton* native_button_;

        // It's expensive to find the size of a button on windows, so we cache it.
        mutable gfx::Size button_size_;
        mutable bool button_size_valid_;

        DISALLOW_COPY_AND_ASSIGN(NativeButtonWin);
    };

} //namespace view

#endif //__view_native_button_win_h__