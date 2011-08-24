
#ifndef __view_html_view_h__
#define __view_html_view_h__

#include <mshtml.h>

#include "view/activex/ax_host.h"
#include "view/view.h"

namespace view
{

    class HtmlView : public View, public AxHostDelegate
    {
    public:
        explicit HtmlView();
        virtual ~HtmlView();

        bool SetHtml(const std::string& html);

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
        base::win::ScopedComPtr<IHTMLDocument2> html_document_;

        bool initialized_;
    };

} //namespace view

#endif //__view_html_view_h__