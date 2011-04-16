
#ifndef __view_menu_model_delegate_h__
#define __view_menu_model_delegate_h__

#pragma once

namespace view
{

    class MenuModelDelegate
    {
    public:
        // Invoked when an icon has been loaded from history.
        virtual void OnIconChanged(int index) = 0;

    protected:
        virtual ~MenuModelDelegate() {}
    };

} //namespace view

#endif //__view_menu_model_delegate_h__