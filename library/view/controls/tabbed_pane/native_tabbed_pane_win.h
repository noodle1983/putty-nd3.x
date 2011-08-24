
#ifndef __view_native_tabbed_pane_win_h__
#define __view_native_tabbed_pane_win_h__

#pragma once

#include <vector>

#include "native_tabbed_pane_wrapper.h"
#include "view/controls/native_control_win.h"

namespace view
{

    class Widget;
    class TabLayout;

    class NativeTabbedPaneWin : public NativeControlWin,
        public NativeTabbedPaneWrapper
    {
    public:
        explicit NativeTabbedPaneWin(TabbedPane* tabbed_pane);
        virtual ~NativeTabbedPaneWin();

        // NativeTabbedPaneWrapper implementation:
        virtual void AddTab(const std::wstring& title, View* contents);
        virtual void AddTabAtIndex(int index,
            const std::wstring& title,
            View* contents,
            bool select_if_first_tab);
        virtual View* RemoveTabAtIndex(int index);
        virtual void SelectTabAt(int index);
        virtual int GetTabCount();
        virtual int GetSelectedTabIndex();
        virtual View* GetSelectedTab();
        virtual View* GetView();
        virtual void SetFocus();
        virtual gfx::Size GetPreferredSize();
        virtual HWND GetTestingHandle() const;

        // NativeControlWin overrides.
        virtual void CreateNativeControl();
        virtual bool ProcessMessage(UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT* result);

        // View overrides:
        virtual void Layout();
        virtual FocusTraversable* GetFocusTraversable();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);

    private:
        // Called upon creation of native control to initialize tabs that are added
        // before the native control is created.
        void InitializeTabs();

        // Adds a tab with the given content to native control at the given index.
        void AddNativeTab(int index, const std::wstring& title);

        // Changes the contents view to the view associated with the tab at |index|.
        // |invoke_listener| controls if this methold should invoke the
        // Listener::TabSelectedAt callback.
        void DoSelectTabAt(int index, boolean invoke_listener);

        // Resizes the HWND control to macth the size of the containing view.
        void ResizeContents();

        // The tabbed-pane we are bound to.
        TabbedPane* tabbed_pane_;

        // The layout manager we use for managing our tabs.
        TabLayout* tab_layout_manager_;

        // The views associated with the different tabs.
        std::vector<View*> tab_views_;

        // The tab's title strings.
        std::vector<const std::wstring> tab_titles_;

        // The index of the selected tab.
        int selected_index_;

        // The window displayed in the tab.
        Widget* content_window_;

        DISALLOW_COPY_AND_ASSIGN(NativeTabbedPaneWin);
    };

} //namespace view

#endif //__view_native_tabbed_pane_win_h__