
#ifndef __ui_base_combobox_model_h__
#define __ui_base_combobox_model_h__

#pragma once

#include "base/string16.h"

namespace ui
{

    // The interface for models backing a combobox.
    class ComboboxModel
    {
    public:
        virtual ~ComboboxModel() {}

        // Return the number of items in the combo box.
        virtual int GetItemCount() = 0;

        // Return the string that should be used to represent a given item.
        virtual string16 GetItemAt(int index) = 0;
    };

} //namespace ui

#endif //__ui_base_combobox_model_h__