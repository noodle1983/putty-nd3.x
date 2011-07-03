
#ifndef __wan_chrome_frame_browser_view_h__
#define __wan_chrome_frame_browser_view_h__

#pragma once

#include "view/view/client_view.h"
#include "view/window/window_delegate.h"

class BrowserFrameWin;

class BrowserView : public view::WindowDelegate, public view::ClientView
{
public:
    static const char kViewClassName[];

    explicit BrowserView();
    virtual ~BrowserView();

    void set_frame(BrowserFrameWin* frame) { frame_ = frame; }
    BrowserFrameWin* frame() const { return frame_; }

    // Returns a pointer to the BrowserView* interface implementation (an
    // instance of this object, typically) for a given native window, or NULL if
    // there is no such association.
    static BrowserView* GetBrowserViewForNativeWindow(HWND window);

    // Returns the bounds of the content area, in the coordinates of the
    // BrowserView's parent.
    gfx::Rect GetClientAreaBounds() const;

    // Overridden from view::WindowDelegate:
    virtual bool CanResize() const;
    virtual bool CanMaximize() const;
    virtual bool CanActivate() const;
    virtual bool IsModal() const;
    virtual std::wstring GetWindowTitle() const;
    virtual std::wstring GetAccessibleWindowTitle() const;
    virtual view::View* GetInitiallyFocusedView();
    virtual bool ShouldShowWindowTitle() const;
    virtual SkBitmap GetWindowAppIcon();
    virtual SkBitmap GetWindowIcon();
    virtual bool ShouldShowWindowIcon() const;
    virtual view::View* GetContentsView();
    virtual view::ClientView* CreateClientView(view::Window* window);

    // Overridden from view::ClientView:
    virtual bool CanClose();
    virtual int NonClientHitTest(const gfx::Point& point);
    virtual gfx::Size GetMinimumSize();

    // Overridden from view::View:
    virtual std::string GetClassName() const;
    virtual void Layout();
    virtual void PaintChildren(gfx::Canvas* canvas);
    virtual void ViewHierarchyChanged(bool is_add,
        view::View* parent, view::View* child);
    virtual void ChildPreferredSizeChanged(View* child);
    virtual void GetAccessibleState(AccessibleViewState* state);

    // Factory Methods.
    // Returns a new LayoutManager for this browser view. A subclass may
    // override to implemnet different layout pocily.
    virtual view::LayoutManager* CreateLayoutManager() const;

    // Browser window related initializations.
    virtual void Init();

private:
    friend class BrowserViewLayout;

    // Returns the BrowserViewLayout.
    BrowserViewLayout* GetBrowserViewLayout() const;

    // The BrowserFrame that hosts this view.
    BrowserFrameWin* frame_;

    // The view managing both the contents_container_ and preview_container_.
    view::View* contents_;

    // True if we have already been initialized.
    bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(BrowserView);
};

#endif //__wan_chrome_frame_browser_view_h__