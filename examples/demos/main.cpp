
#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>

#include <atlbase.h>

#include "../wanui_res/resource.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/path_service.h"

#include "SkBitmap.h"

#include "ui_gfx/image/image.h"
#include "ui_gfx/transform.h"

#include "ui_base/clipboard/clipboard.h"
#include "ui_base/models/combobox_model.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/win/hwnd_util.h"

#include "view/controls/button/checkbox.h"
#include "view/controls/button/radio_button.h"
#include "view/controls/button/text_button.h"
#include "view/focus/accelerator_handler.h"
#include "view/view.h"
#include "view/view_delegate.h"
#include "view/widget/widget.h"
#include "view/widget/widget_delegate.h"

CComModule _Module;

class TestViewDelegate : public view::ViewDelegate
{
public:
    TestViewDelegate() {}
    virtual ~TestViewDelegate() {}

    // Overridden from view::ViewsDelegate:
    virtual ui::Clipboard* GetClipboard() const
    {
        if(!clipboard_.get())
        {
            // Note that we need a MessageLoop for the next call to work.
            clipboard_.reset(new ui::Clipboard);
        }
        return clipboard_.get();
    }
    virtual void SaveWindowPlacement(const view::Widget* widget,
        const std::wstring& window_name,
        const gfx::Rect& bounds,
        bool maximized) {}
    virtual bool GetSavedWindowBounds(const view::Widget* widget,
        const std::wstring& window_name,
        gfx::Rect* bounds) const { return false; }

    virtual bool GetSavedMaximizedState(const view::Widget* widget,
        const std::wstring& window_name,
        bool* maximized) const { return false; }

    virtual void NotifyAccessibilityEvent(view::View* view,
        ui::AccessibilityTypes::Event event_type) {}

    virtual void NotifyMenuItemFocused(
        const std::wstring& menu_name,
        const std::wstring& menu_item_name,
        int item_index,
        int item_count,
        bool has_submenu) {}

    virtual HICON GetDefaultWindowIcon() const
    {
        return NULL;
    }

    view::View* GetDefaultParentView()
    {
        return NULL;
    }

    virtual void AddRef() {}
    virtual void ReleaseRef() {}

    virtual int GetDispositionForEvent(int event_flags) { return 0; }

private:
    mutable scoped_ptr<ui::Clipboard> clipboard_;

    DISALLOW_COPY_AND_ASSIGN(TestViewDelegate);
};

class MainView : public view::WidgetDelegateView
{
public:
    MainView()
    {
        set_background(view::Background::CreateSolidBackground(SK_ColorWHITE));
    }
    virtual ~MainView() {}

    // Overridden from view::WidgetDelegate:
    virtual std::wstring GetWindowTitle() const
    {
        return L"Demos";
    }

    virtual void WindowClosing()
    {
        MessageLoopForUI::current()->Quit();
    }

    virtual view::View* GetContentsView()
    {
        return this;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MainView);
};

// 程序入口.
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hRes = OleInitialize(NULL);

    // this resolves ATL window thunking problem when Microsoft Layer
    // for Unicode (MSLU) is used.
    ::DefWindowProc(NULL, 0, 0, 0L);

    base::AtExitManager exit_manager;
    CommandLine::Init(0, NULL);

    FilePath res_dll;
    PathProvider(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    view::ViewDelegate::view_delegate = new TestViewDelegate();
    
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);
    MainView* main_view = new MainView();
    view::Widget::CreateWindow(main_view);
    ui::CenterAndSizeWindow(NULL,
        main_view->GetWidget()->GetNativeWindow(),
        gfx::Size(800, 600), false);
    main_view->GetWidget()->Show();
    MessageLoopForUI::current()->Run(&handler);

    delete view::ViewDelegate::view_delegate;
    view::ViewDelegate::view_delegate = NULL;

    ui::ResourceBundle::CleanupSharedInstance();
    OleUninitialize();

    return 0;
}


// 提升公共控件样式.
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif