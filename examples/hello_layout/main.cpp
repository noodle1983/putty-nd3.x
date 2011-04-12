
#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>

#include <atlbase.h>

#include "../wanui_res/resource.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/memory/scoped_ptr.h"

#include "message/message_loop.h"

#include "view/base/hwnd_util.h"
#include "view/base/resource_bundle.h"
#include "view/controls/button/text_button.h"
#include "view/focus/accelerator_handler.h"
#include "view/layout/box_layout.h"
#include "view/layout/fill_layout.h"
#include "view/layout/grid_layout.h"
#include "view/view/view.h"
#include "view/window/window_delegate.h"
#include "view/window/window.h"

CComModule _Module;

// 主窗口代理.
class MainWindow : public view::WindowDelegate
{
    view::View* content_;

public:
    MainWindow() : content_(NULL) {}

    virtual bool CanResize() const
    {
        return true;
    }

    virtual bool CanMaximize() const
    {
        return true;
    }

    virtual std::wstring GetWindowTitle() const
    {
        return L"Hello, Layout!";
    }

    virtual void WindowClosing()
    {
        MessageLoopForUI::current()->Quit();
    }

    virtual void DeleteDelegate() { delete this; }

    virtual void OnWindowBeginUserBoundsChange()
    {
        window()->SetUseDragFrame(true);
    }

    virtual void OnWindowEndUserBoundsChange()
    {
        window()->SetUseDragFrame(false);
    }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->set_background(view::Background::CreateStandardPanelBackground());

            view::GridLayout* grid_layout = view::GridLayout::CreatePanel(content_);
            content_->SetLayoutManager(grid_layout);

            view::ColumnSet* column_set = grid_layout->AddColumnSet(0);
            column_set->AddColumn(view::GridLayout::LEADING, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);
            column_set = grid_layout->AddColumnSet(1);
            column_set->AddColumn(view::GridLayout::CENTER, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);
            column_set = grid_layout->AddColumnSet(2);
            column_set->AddColumn(view::GridLayout::TRAILING, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);
            column_set = grid_layout->AddColumnSet(3);
            column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);
            column_set = grid_layout->AddColumnSet(4);
            column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);
            column_set = grid_layout->AddColumnSet(5);
            column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
                1, view::GridLayout::USE_PREF, 0, 0);

            {
                grid_layout->StartRow(0, 0);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(100, 222, 231));
                v->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kHorizontal, 0, 0, 5));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 1);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(100, 222, 231));
                v->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kHorizontal, 0, 0, 5));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 2);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(100, 222, 231));
                v->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kHorizontal, 0, 0, 5));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 3);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(100, 222, 231));
                v->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kHorizontal, 0, 0, 5));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                v->AddChildView(new view::TextButton(NULL, L"Button In BoxLayout"));
                grid_layout->AddView(v);
            }

            {
                grid_layout->StartRow(0, 0);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(222, 100, 231));
                v->SetLayoutManager(new view::FillLayout());
                v->AddChildView(new view::TextButton(NULL, L"Button In FillLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 1);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(222, 100, 231));
                v->SetLayoutManager(new view::FillLayout());
                v->AddChildView(new view::TextButton(NULL, L"Button In FillLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 2);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(222, 100, 231));
                v->SetLayoutManager(new view::FillLayout());
                v->AddChildView(new view::TextButton(NULL, L"Button In FillLayout"));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0, 3);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(222, 100, 231));
                v->SetLayoutManager(new view::FillLayout());
                v->AddChildView(new view::TextButton(NULL, L"Button In FillLayout"));
                grid_layout->AddView(v);
            }

            {
                grid_layout->StartRow(0.3, 4);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(222, 100, 100));
                grid_layout->AddView(v);
            }
            {
                grid_layout->StartRow(0.7, 5);
                view::View* v = new view::View();
                v->set_background(view::Background::CreateSolidBackground(100, 100, 222));
                grid_layout->AddView(v);
            }
        }

        return content_;
    }
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
    base::CommandLine::Init(0, NULL);

    FilePath res_dll;
    PathProvider(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ResourceBundle::InitSharedInstance(res_dll);
    
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);
    MainWindow* main_window = new MainWindow();
    view::Window::CreateWanWindow(NULL, gfx::Rect(0, 0, 300, 300), main_window);
    view::CenterAndSizeWindow(NULL, main_window->window()->GetNativeWindow(),
        main_window->window()->GetBounds().size(), false);
    main_window->window()->Show();
    MessageLoopForUI::current()->Run(&handler);

    ResourceBundle::CleanupSharedInstance();
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