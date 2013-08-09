#ifndef PUTTY_VIEW_H
#define PUTTY_VIEW_H

#include <algorithm>
#include "view/view.h"

class NativePuttyController;
namespace view
{

    // 实现无窗口的richedit控件.
    class PuttyView : public View
	{
	public:
		static const char kViewClassName[];

        explicit PuttyView();
        virtual ~PuttyView();

		// This method is invoked when the tree changes.
        //
        // When a view is removed, it is invoked for all children and grand
        // children. For each of these views, a notification is sent to the
        // view and all parents.
        //
        // When a view is added, a notification is sent to the view, all its
        // parents, and all its children (and grand children)
        //
        // Default implementation does nothing. Override to perform operations
        // required when a view is added or removed from a view hierarchy
        //
        // parent is the new or old parent. Child is the view being added or
        // removed.
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);

        // When SetVisible() changes the visibility of a view, this method is
        // invoked for that view as well as all the children recursively.
        virtual void VisibilityChanged(View* starting_from, bool is_visible);
		void OnFocus();
		virtual bool OnKeyPressed(const KeyEvent& event);
		bool SkipDefaultKeyEventProcessing(const KeyEvent& event)
		{
			return event.key_code() == ui::VKEY_TAB;
		}
		
		virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
		virtual void OnPaint(gfx::Canvas* canvas);

		string16& getWinTitle();
		HWND getNativeView();
		bool isLoading();
		bool isDisconnected();
		void dupCfg2Global();
		void do_copy();
		void do_paste();
		void do_restart();
		void do_reconfig();
		void do_copyAll();
		void do_clearScrollbar();
		void do_log(bool isPressed);
		void do_shortcutEnabler(bool isPressed);
		bool isLogStarted();
		bool isShortcutEnabled();
		void searchNext(const string16& str) ;
		void searchPrevious(const string16& str) ;
		void resetSearch() ;
		void setFocus();
	protected:
        virtual void Layout();
		virtual void Paint(gfx::Canvas* canvas);
		NativePuttyController* puttyController_;

	};

}

#endif