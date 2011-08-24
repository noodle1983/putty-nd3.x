
#ifndef __view_listbox_model_h__
#define __view_listbox_model_h__

#pragma once

namespace view
{

    // The interface for models backing a listbox.
    class ListboxModel
    {
    public:
        virtual ~ListboxModel() {}

        // Return the number of items in the listbox.
        virtual int GetItemCount() = 0;

        // Return the string that should be used to represent a given item.
        virtual string16 GetItemAt(int index) = 0;
    };

} //namespace view

#endif //__view_listbox_model_h__