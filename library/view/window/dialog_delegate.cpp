
#include "dialog_delegate.h"

#include "base/logging.h"

#include "../controls/button/native_button.h"
#include "window.h"

namespace view
{

    // Overridden from WindowDelegate:

    DialogDelegate* DialogDelegate::AsDialogDelegate() { return this; }

    int DialogDelegate::GetDialogButtons() const
    {
        return MessageBoxFlags::DIALOGBUTTON_OK |
            MessageBoxFlags::DIALOGBUTTON_CANCEL;
    }

    bool DialogDelegate::AreAcceleratorsEnabled(
        MessageBoxFlags::DialogButton button)
    {
        return true;
    }

    std::wstring DialogDelegate::GetDialogButtonLabel(
        MessageBoxFlags::DialogButton button) const
    {
        // empty string results in defaults for
        // MessageBoxFlags::DIALOGBUTTON_OK,
        // MessageBoxFlags::DIALOGBUTTON_CANCEL.
        return L"";
    }

    View* DialogDelegate::GetExtraView()
    {
        return NULL;
    }

    bool DialogDelegate::GetSizeExtraViewHeightToButtons()
    {
        return false;
    }

    int DialogDelegate::GetDefaultDialogButton() const
    {
        if(GetDialogButtons() & MessageBoxFlags::DIALOGBUTTON_OK)
        {
            return MessageBoxFlags::DIALOGBUTTON_OK;
        }
        if(GetDialogButtons() & MessageBoxFlags::DIALOGBUTTON_CANCEL)
        {
            return MessageBoxFlags::DIALOGBUTTON_CANCEL;
        }
        return MessageBoxFlags::DIALOGBUTTON_NONE;
    }

    bool DialogDelegate::IsDialogButtonEnabled(
        MessageBoxFlags::DialogButton button) const
    {
        return true;
    }

    bool DialogDelegate::IsDialogButtonVisible(
        MessageBoxFlags::DialogButton button) const
    {
        return true;
    }

    bool DialogDelegate::Cancel()
    {
        return true;
    }

    bool DialogDelegate::Accept(bool window_closiang)
    {
        return Accept();
    }

    bool DialogDelegate::Accept()
    {
        return true;
    }

    View* DialogDelegate::GetInitiallyFocusedView()
    {
        // Focus the default button if any.
        DialogClientView* dcv = GetDialogClientView();
        int default_button = GetDefaultDialogButton();
        if(default_button == MessageBoxFlags::DIALOGBUTTON_NONE)
        {
            return NULL;
        }

        if((default_button & GetDialogButtons()) == 0)
        {
            // The default button is a button we don't have.
            NOTREACHED();
            return NULL;
        }

        if(default_button & MessageBoxFlags::DIALOGBUTTON_OK)
        {
            return dcv->ok_button();
        }
        if(default_button & MessageBoxFlags::DIALOGBUTTON_CANCEL)
        {
            return dcv->cancel_button();
        }
        return NULL;
    }

    ClientView* DialogDelegate::CreateClientView(Window* window)
    {
        return new DialogClientView(window, GetContentsView());
    }

    DialogClientView* DialogDelegate::GetDialogClientView() const
    {
        return window()->client_view()->AsDialogClientView();
    }

    AccessibilityTypes::Role DialogDelegate::GetAccessibleWindowRole() const
    {
        return AccessibilityTypes::ROLE_DIALOG;
    }

} //namespace view