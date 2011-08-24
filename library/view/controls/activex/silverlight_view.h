
#ifndef __view_silverlight_view_h__
#define __view_silverlight_view_h__

#include "xcp_ctrl.h"

#include "view/activex/ax_host.h"
#include "view/view.h"

namespace view
{

    class SilverlightView : public IPropertyBag, public IPropertyBag2,
        public View, public AxHostDelegate, public IXcpControlHost2
    {
    public:
        explicit SilverlightView();
        virtual ~SilverlightView();

        IXcpControl* xcp_control()  const { return xcp_control_; }
        bool Play(const std::wstring& url);

        // Overridden from View:
        virtual bool OnSetCursor(const gfx::Point& p);
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual void OnMouseMoved(const MouseEvent& event);
        virtual bool OnKeyPressed(const KeyEvent& event);
        virtual bool OnKeyReleased(const KeyEvent& event);
        virtual LRESULT OnImeMessages(UINT message, WPARAM w_param,
            LPARAM l_param, BOOL* handled);

    protected:
        // IUnknown:
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID iid, void** object);

        // IPropertyBag
        STDMETHOD(Read)(LPCOLESTR pszPropName,
            VARIANT* pVar,
            IErrorLog* pErrorLog);
        STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar);

        // IPropertyBag2
        STDMETHOD(Read)(ULONG cProperties,
            PROPBAG2* pPropBag,
            IErrorLog* pErrLog,
            VARIANT* pvarValue,
            HRESULT* phrError);
        STDMETHOD(Write)(ULONG cProperties,
            PROPBAG2* pPropBag,
            VARIANT* pvarValue);
        STDMETHOD(CountProperties)(ULONG* pcProperties);
        STDMETHOD(GetPropertyInfo)(ULONG iProperty,
            ULONG cProperties,
            PROPBAG2* pPropBag,
            ULONG* pcProperties);
        STDMETHOD(LoadObject)(LPCOLESTR pstrName,
            DWORD dwHint,
            IUnknown* pUnkObject,
            IErrorLog* pErrLog);

        // Overridden from View:
        virtual void OnPaint(gfx::Canvas* canvas);
        virtual void Layout();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual void OnFocus();
        virtual void OnBlur();

        // Overridden from AxHostDelegate:
        virtual HWND GetAxHostWindow() const;
        virtual void OnAxCreate(AxHost* host);
        virtual void OnAxInvalidate(const gfx::Rect& rect);
        virtual HRESULT QueryService(REFGUID guidService,
            REFIID riid, void** ppvObject);

        // IXcpControlHost
        STDMETHOD(GetHostOptions)(DWORD* pdwOptions);
        STDMETHOD(NotifyLoaded)();
        STDMETHOD(NotifyError)(BSTR bstrError,
            BSTR bstrSource,
            long nLine,
            long nColumn);
        STDMETHOD(InvokeHandler)(BSTR bstrName,
            VARIANT varArg1,
            VARIANT varArg2,
            VARIANT* pvarResult);
        STDMETHOD(GetBaseUrl)(BSTR* pbstrUrl);
        STDMETHOD(GetNamedSource)(BSTR bstrSourceName,
            BSTR* pbstrSource);
        STDMETHOD(DownloadUrl)(BSTR bstrUrl,
            IXcpControlDownloadCallback* pCallback,
            IStream** ppStream);

        // IXcpControlHost2
        STDMETHOD(GetCustomAppDomain)(IUnknown** ppAppDomain);
        STDMETHOD(GetControlVersion)(UINT* puMajorVersion,
            UINT* puMinorVersion);

    private:
        scoped_ptr<AxHost> ax_host_;
        base::win::ScopedComPtr<IXcpControl> xcp_control_;

        bool initialized_;

        struct PropertyParam
        {
            PropertyParam(const std::wstring& name, const std::wstring& value)
            {
                this->name = name;
                this->value = value;
            }
            PropertyParam() {}
            std::wstring name;
            std::wstring value;
        };
        typedef std::vector<PropertyParam> PropertyParams;
        PropertyParams props_;
    };

} //namespace view

#endif //__view_silverlight_view_h__