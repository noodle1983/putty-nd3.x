
#ifndef __dropdown_bar_host_delegate_h__
#define __dropdown_bar_host_delegate_h__

class DropdownBarHostDelegate
{
public:
    // Claims focus for the text field and selects its contents.
    virtual void SetFocusAndSelection(bool select_all) = 0;

    // Updates the view to let it know where the host is clipping the
    // dropdown widget (while animating the opening or closing of the widget).
    virtual void SetAnimationOffset(int offset) = 0;

protected:
    virtual ~DropdownBarHostDelegate() {}
};

#endif //__dropdown_bar_host_delegate_h__