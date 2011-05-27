
#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>

#include <atlbase.h>

#include "resource.h"

#include "../wanui_res/resource.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/stringprintf.h"

#include "message/message_loop.h"

#include "view/base/hwnd_util.h"
#include "view/base/resource_bundle.h"
#include "view/focus/accelerator_handler.h"
#include "view/view/view.h"
#include "view/window/window_delegate.h"
#include "view/window/window.h"

CComModule _Module;

class DragView : public view::View
{
    gfx::Point initial_drag_point_;

public:
    virtual HCURSOR GetCursorForPoint(view::EventType event_type,
        const gfx::Point& p)
    {
        static HCURSOR size_all = LoadCursor(NULL, IDC_SIZEALL);
        return size_all;
    }

    virtual bool OnMousePressed(const view::MouseEvent& event)
    {
        if(!event.IsOnlyLeftMouseButton())
        {
            return false;
        }
        
        initial_drag_point_ = event.location();
        return true;
    }

    virtual bool OnMouseDragged(const view::MouseEvent& event)
    {
        if(!event.IsOnlyLeftMouseButton())
        {
            return false;
        }

        gfx::Rect new_bounds = bounds();
        int delta_x = event.x() - initial_drag_point_.x();
        int delta_y = event.y() - initial_drag_point_.y();
        new_bounds.Offset(delta_x, delta_y);
        SetBoundsRect(new_bounds);
        return true;
    }
};

// 主窗口代理.
class MainWindow : public view::WindowDelegate,
    public view::ContextMenuController
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
        return L"Hello, View!";
    }

    virtual void WindowClosing()
    {
        MessageLoopForUI::current()->Quit();
    }

    virtual void DeleteDelegate() { delete this; }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->set_background(
                view::Background::CreateSolidBackground(109, 221, 231));
            content_->SetContextMenuController(this);
        }

        return content_;
    }

    virtual void ShowContextMenuForView(view::View* source,
        const gfx::Point& p,
        bool is_mouse_gesture)
    {
        HMENU main_menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_MAIN));
        if(main_menu)
        {
            HMENU popup_menu = GetSubMenu(main_menu, 0);
            int id = TrackPopupMenu(popup_menu,
                TPM_LEFTBUTTON|TPM_RIGHTBUTTON|TPM_RETURNCMD,
                p.x(), p.y(), 0, window()->GetNativeWindow(), NULL);
            // 处理菜单命令.
            switch(id)
            {
            case ID_RED:
                content_->set_background(
                    view::Background::CreateSolidBackground(255, 0, 0));
                content_->SchedulePaint();
                break;
            case ID_GREEN:
                content_->set_background(
                    view::Background::CreateSolidBackground(0, 255, 0));
                content_->SchedulePaint();
                break;
            case ID_BLUE:
                content_->set_background(
                    view::Background::CreateSolidBackground(0, 0, 255));
                content_->SchedulePaint();
                break;
            case ID_GRADIENT:
                content_->set_background(
                    view::Background::CreateVerticalGradientBackground(
                    SkColorSetRGB(0, 255, 0), SkColorSetRGB(0, 0, 255)));
                content_->SchedulePaint();
                break;
            case ID_STANDARD_PANEL:
                content_->set_background(
                    view::Background::CreateStandardPanelBackground());
                content_->SchedulePaint();
                break;
            case ID_BORDER:
                content_->set_border(view::Border::CreateSolidBorder(2,
                    SkColorSetRGB(127, 127, 127)));
                content_->SchedulePaint();
                break;
            case ID_CREATE:
                {
                    view::View* new_view = new DragView();
                    new_view->SetContextMenuController(this);
                    new_view->set_background(view::Background::CreateSolidBackground(
                        SkColorSetRGB(base::RandInt(0, 255),
                        base::RandInt(0, 255),
                        base::RandInt(0, 255))));
                    gfx::Rect content_bounds = content_->GetContentsBounds();
                    new_view->SetBounds(
                        base::RandInt(content_bounds.x(), content_bounds.right()),
                        base::RandInt(content_bounds.y(), content_bounds.bottom()),
                        100, 100);
                    content_->AddChildView(new_view);
                    new_view->SchedulePaint();
                }
                break;
            case ID_BRING_TOP:
                if(source != content_)
                {
                    content_->RemoveChildView(source);
                    content_->AddChildView(source);
                    content_->SchedulePaintInRect(source->bounds());
                }
                break;
            case ID_DELETE:
                if(source != content_)
                {
                    content_->RemoveChildView(source);
                    content_->SchedulePaintInRect(source->bounds());
                    delete source;
                }
                break;
            case ID_REMOVE_ALL:
                {
                    content_->RemoveAllChildViews(true);
                    content_->SchedulePaint();
                }
                break;
            case ID_HIDE_ALL:
                for(int i=0; i<content_->child_count(); ++i)
                {
                    content_->GetChildViewAt(i)->SetVisible(false);
                }
                break;
            case ID_SHOW_ALL:
                for(int i=0; i<content_->child_count(); ++i)
                {
                    content_->GetChildViewAt(i)->SetVisible(true);
                }
                break;
            }
            DestroyMenu(main_menu);
        }
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