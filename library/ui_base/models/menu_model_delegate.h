
#ifndef __ui_base_menu_model_delegate_h__
#define __ui_base_menu_model_delegate_h__

#pragma once

namespace ui
{

    class MenuModelDelegate
    {
    public:
        // Invoked when an icon has been loaded from history.
        virtual void OnIconChanged(int index) = 0;

    protected:
        virtual ~MenuModelDelegate() {}
    };

} //namespace ui

#endif //__ui_base_menu_model_delegate_h__