
#ifndef __ui_base_simple_menu_model_h__
#define __ui_base_simple_menu_model_h__

#pragma once

#include <vector>

#include "base/task.h"

#include "menu_model.h"


namespace ui
{

    class ButtonMenuItemModel;

    // A simple MenuModel implementation with an imperative API for adding menu
    // items. This makes it easy to construct fixed menus. Menus populated by
    // dynamic data sources may be better off implementing MenuModel directly.
    // The breadth of MenuModel is not exposed through this API.
    class SimpleMenuModel : public MenuModel
    {
    public:
        class Delegate
        {
        public:
            // Methods for determining the state of specific command ids.
            virtual bool IsCommandIdChecked(int command_id) const = 0;
            virtual bool IsCommandIdEnabled(int command_id) const = 0;
            virtual bool IsCommandIdVisible(int command_id) const;

            // Gets the accelerator for the specified command id. Returns true if the
            // command id has a valid accelerator, false otherwise.
            virtual bool GetAcceleratorForCommandId(int command_id,
                Accelerator* accelerator) = 0;

            // Some command ids have labels and icons that change over time.
            virtual bool IsItemForCommandIdDynamic(int command_id) const;
            virtual string16 GetLabelForCommandId(int command_id) const;
            // Gets the icon for the item with the specified id, returning true if there
            // is an icon, false otherwise.
            virtual bool GetIconForCommandId(int command_id, SkBitmap* icon) const;

            // Notifies the delegate that the item with the specified command id was
            // visually highlighted within the menu.
            virtual void CommandIdHighlighted(int command_id);

            // Performs the action associated with the specified command id.
            virtual void ExecuteCommand(int command_id) = 0;

            // Notifies the delegate that the menu is about to show.
            virtual void MenuWillShow(SimpleMenuModel* source);

            // Notifies the delegate that the menu has closed.
            virtual void MenuClosed(SimpleMenuModel* source);

        protected:
            virtual ~Delegate() {}
        };

        // The Delegate can be NULL, though if it is items can't be checked or
        // disabled.
        explicit SimpleMenuModel(Delegate* delegate);
        virtual ~SimpleMenuModel();

        // Methods for adding items to the model.
        void AddItem(int command_id, const string16& label);
        void AddItemWithStringId(int command_id, int string_id);
        void AddSeparator();
        void AddCheckItem(int command_id, const string16& label);
        void AddCheckItemWithStringId(int command_id, int string_id);
        void AddRadioItem(int command_id, const string16& label, int group_id);
        void AddRadioItemWithStringId(int command_id, int string_id, int group_id);

        // These two methods take pointers to various sub-models. These models
        // should be owned by the same owner of this SimpleMenuModel.
        void AddSubMenu(int command_id, const string16& label, MenuModel* model);
        void AddSubMenuWithStringId(int command_id, int string_id, MenuModel* model);

        // Methods for inserting items into the model.
        void InsertItemAt(int index, int command_id, const string16& label);
        void InsertItemWithStringIdAt(int index, int command_id, int string_id);
        void InsertSeparatorAt(int index);
        void InsertCheckItemAt(int index, int command_id, const string16& label);
        void InsertCheckItemWithStringIdAt(int index, int command_id, int string_id);
        void InsertRadioItemAt(int index, int command_id, const string16& label,
            int group_id);
        void InsertRadioItemWithStringIdAt(int index, int command_id,
            int string_id, int group_id);
        void InsertSubMenuAt(int index, int command_id, const string16& label,
            MenuModel* model);
        void InsertSubMenuWithStringIdAt(int index, int command_id, int string_id,
            MenuModel* model);

        // Sets the icon for the item at |index|.
        void SetIcon(int index, const SkBitmap& icon);

        // Clears all items. Note that it does not free MenuModel of submenu.
        void Clear();

        // Returns the index of the item that has the given |command_id|. Returns
        // -1 if not found.
        int GetIndexOfCommandId(int command_id);

        // Overridden from MenuModel:
        virtual bool HasIcons() const;
        virtual int GetItemCount() const;
        virtual ItemType GetTypeAt(int index) const;
        virtual int GetCommandIdAt(int index) const;
        virtual string16 GetLabelAt(int index) const;
        virtual bool IsItemDynamicAt(int index) const;
        virtual bool GetAcceleratorAt(int index,
            Accelerator* accelerator) const;
        virtual bool IsItemCheckedAt(int index) const;
        virtual int GetGroupIdAt(int index) const;
        virtual bool GetIconAt(int index, SkBitmap* icon) const;
        virtual ButtonMenuItemModel* GetButtonMenuItemAt(int index) const;
        virtual bool IsEnabledAt(int index) const;
        virtual bool IsVisibleAt(int index) const;
        virtual void HighlightChangedTo(int index);
        virtual void ActivatedAt(int index);
        virtual MenuModel* GetSubmenuModelAt(int index) const;
        virtual void MenuWillShow();
        virtual void MenuClosed();
        virtual void SetMenuModelDelegate(MenuModelDelegate* menu_model_delegate);

    protected:
        // Some variants of this model (SystemMenuModel) relies on items to be
        // inserted backwards. This is counter-intuitive for the API, so rather than
        // forcing customers to insert things backwards, we return the indices
        // backwards instead. That's what this method is for. By default, it just
        // returns what it's passed.
        virtual int FlipIndex(int index) const;

        Delegate* delegate() { return delegate_; }

        MenuModelDelegate* menu_model_delegate() { return menu_model_delegate_; }

    private:
        struct Item;

        // Functions for inserting items into |items_|.
        void AppendItem(const Item& item);
        void InsertItemAtIndex(const Item& item, int index);
        void ValidateItem(const Item& item);

        // Notify the delegate that the menu is closed.
        void OnMenuClosed();

        std::vector<Item> items_;

        Delegate* delegate_;

        MenuModelDelegate* menu_model_delegate_;

        ScopedRunnableMethodFactory<SimpleMenuModel> method_factory_;

        DISALLOW_COPY_AND_ASSIGN(SimpleMenuModel);
    };

} //namespace ui

#endif //__ui_base_simple_menu_model_h__