
#ifndef __view_webbrowser_view_h__
#define __view_webbrowser_view_h__

#include <exdisp.h>

#include "view/activex/ax_host.h"
#include "view/view.h"

namespace view
{

    class WebBrowserView : public View, public AxHostDelegate
    {
    public:
        explicit WebBrowserView();
        virtual ~WebBrowserView();

        IWebBrowser2* webbrowser() const { return webbrowser_; }
        bool Navigate(const std::wstring& url);

    protected:
        // Overridden from View:
        virtual void Layout();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual void VisibilityChanged(View* starting_from, bool is_visible);

        // Overridden from AxHostDelegate:
        virtual HWND GetAxHostWindow() const;
        virtual void OnAxCreate(AxHost* host);
        virtual void OnAxInvalidate(const gfx::Rect& rect);

    private:
        scoped_ptr<AxHost> ax_host_;
        base::win::ScopedComPtr<IWebBrowser2> webbrowser_;

        bool initialized_;
    };

} //namespace view

#endif //__view_webbrowser_view_h__