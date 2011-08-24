
#ifndef __demo_list_h__
#define __demo_list_h__

#pragma once

#include "ui_base/models/combobox_model.h"
#include "view/controls/combobox/combobox.h"
#include "view/controls/listbox/listbox_model.h"
#include "view/controls/listbox/listbox.h"

#include "demo_base.h"

class DemoList : public DemoBase, public ui::ComboboxModel,
    public view::Combobox::Listener,
    public view::ListboxModel,
    public view::Listbox::Listener
{
public:
    explicit DemoList(DemoMain* main);
    virtual ~DemoList();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from ui::ComboboxModel & view::ListboxModel:
    virtual int GetItemCount();
    virtual string16 GetItemAt(int index);

    // Overridden from view::Combobox::Listener:
    virtual void ItemChanged(view::Combobox* combo_box,
        int prev_index, int new_index);

    // Overridden from view::Listbox::Listener:
    virtual void SelectionChanged(view::Listbox* sender);

private:
    view::Combobox* combobox_;
    view::Listbox* listbox_;

    DISALLOW_COPY_AND_ASSIGN(DemoList);
};

#endif //__demo_list_h__