
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

#include "gfx/Image.h"

#include "SkBitmap.h"

#include "view/base/hwnd_util.h"
#include "view/base/resource_bundle.h"
#include "view/clipboard/clipboard.h"
#include "view/controls/button/text_button.h"
#include "view/controls/combobox/combobox.h"
#include "view/controls/combobox/combobox_model.h"
#include "view/controls/listbox/listbox.h"
#include "view/controls/listbox/listbox_model.h"
#include "view/controls/tree/tree_view.h"
#include "view/controls/textfield/textfield.h"
#include "view/focus/accelerator_handler.h"
#include "view/gfx/painter.h"
#include "view/layout/box_layout.h"
#include "view/layout/box_layout.h"
#include "view/view/view.h"
#include "view/view/view_delegate.h"
#include "view/widget/widget.h"
#include "view/window/dialog_delegate.h"
#include "view/window/window.h"

CComModule _Module;

// 居中对齐布局器.
class CenterLayout : public view::LayoutManager
{
public:
    CenterLayout() {}
    virtual ~CenterLayout() {}

    virtual void Layout(view::View* host)
    {
        view::View* child = host->GetChildViewAt(0);
        gfx::Size size = child->GetPreferredSize();
        child->SetBounds((host->width()-size.width())/2,
            (host->height()-size.height())/2,
            size.width(), size.height());
    }

    virtual gfx::Size GetPreferredSize(view::View* host)
    {
        return host->GetPreferredSize();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CenterLayout);
};

class TestViewsDelegate : public view::ViewDelegate
{
public:
    TestViewsDelegate() {}
    virtual ~TestViewsDelegate() {}

    // Overridden from views::ViewsDelegate:
    virtual view::Clipboard* GetClipboard() const
    {
        if(!clipboard_.get())
        {
            // Note that we need a MessageLoop for the next call to work.
            clipboard_.reset(new view::Clipboard);
        }
        return clipboard_.get();
    }
    virtual void SaveWindowPlacement(view::Window* window,
        const std::wstring& window_name,
        const gfx::Rect& bounds,
        bool maximized) {}
    virtual bool GetSavedWindowBounds(view::Window* window,
        const std::wstring& window_name,
        gfx::Rect* bounds) const { return false; }

    virtual bool GetSavedMaximizedState(view::Window* window,
        const std::wstring& window_name,
        bool* maximized) const { return false; }

    virtual void NotifyAccessibilityEvent(view::View* view,
        AccessibilityTypes::Event event_type) {}

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

    virtual void AddRef() {}
    virtual void ReleaseRef() {}

private:
    mutable scoped_ptr<view::Clipboard> clipboard_;

    DISALLOW_COPY_AND_ASSIGN(TestViewsDelegate);
};

// 对话框窗口代理.
static const string16 items_data[] =
{
    L"AAA",
    L"BBB",
    L"CCC",
    L"DDD"
};

class ComboboxModelImpl : public view::ComboboxModel
{
public:
    // ComboboxModel接口实现
    virtual int GetItemCount()
    {
        return arraysize(items_data);
    }

    virtual string16 GetItemAt(int index)
    {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, arraysize(items_data));
        return items_data[index];
    }
};

class ListboxModelImpl : public view::ListboxModel
{
public:
    // ListboxModel接口实现
    virtual int GetItemCount()
    {
        return arraysize(items_data);
    }

    virtual string16 GetItemAt(int index)
    {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, arraysize(items_data));
        return items_data[index];
    }
};

class TreeModelNodeImpl : public view::TreeModelNode
{
    string16 title_;

public:
    void SetTitle(const string16& title)
    {
        title_ = title;
    }

    // TreeModelNode接口实现
    virtual const string16& GetTitle() const
    {
        return title_;
    }
};

class TreeModelImpl : public view::TreeModel
{
    TreeModelNodeImpl root_;
    TreeModelNodeImpl childs_[5];

public:
    TreeModelImpl()
    {
        root_.SetTitle(L"Root");
        childs_[0].SetTitle(L"AAA");
        childs_[1].SetTitle(L"BBB");
        childs_[2].SetTitle(L"CCC");
        childs_[3].SetTitle(L"DDD");
        childs_[4].SetTitle(L"EEE");
    }

    // TreeModel接口实现
    virtual view::TreeModelNode* GetRoot()
    {
        return &root_;
    }

    virtual int GetChildCount(view::TreeModelNode* parent)
    {
        if(parent == &root_)
        {
            return 5;
        }
        return 0;
    }

    virtual view::TreeModelNode* GetChild(view::TreeModelNode* parent, int index)
    {
        if(parent == &root_)
        {
            return &childs_[index];
        }
        return NULL;
    }

    virtual int GetIndexOf(view::TreeModelNode* parent, view::TreeModelNode* child)
    {
        if(parent == &root_)
        {
            for(int i=0; i<5; ++i)
            {
                if(&childs_[i] == child)
                {
                    return i;
                }
            }
        }

        return -1;
    }

    virtual view::TreeModelNode* GetParent(view::TreeModelNode* node)
    {
        if(node != &root_)
        {
            return &root_;
        }

        return NULL;
    }

    virtual void AddObserver(view::TreeModelObserver* observer)
    {

    }

    virtual void RemoveObserver(view::TreeModelObserver* observer)
    {

    }

    virtual void SetTitle(view::TreeModelNode* node, const string16& title)
    {

    }
};

class ModalDialog : public view::DialogDelegate
{
    view::View* content_;
    ComboboxModelImpl combobox_model_;
    ListboxModelImpl listbox_model_;
    TreeModelImpl tree_model_;

public:
    ModalDialog() : content_(NULL) {}

    virtual bool CanResize() const
    {
        return true;
    }

    virtual bool IsModal() const
    {
        return true;
    }

    virtual std::wstring GetWindowTitle() const
    {
        return L"对话框";
    }

    virtual void DeleteDelegate() { delete this; }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->SetLayoutManager(new view::BoxLayout(
                view::BoxLayout::kVertical, 0, 0, 5));
            view::Textfield* text_filed = new view::Textfield(
                view::Textfield::STYLE_DEFAULT);
            text_filed->SetTextColor(SkColorSetRGB(192, 221, 149));
            content_->AddChildView(text_filed);

            view::Combobox* combo_box = new view::Combobox(&combobox_model_);
            content_->AddChildView(combo_box);
            combo_box->SetSelectedItem(2);

            view::Listbox* listbox = new view::Listbox(&listbox_model_);
            content_->AddChildView(listbox);
            listbox->SelectRow(2);

            view::TreeView* tree_view = new view::TreeView();
            tree_view->SetModel(&tree_model_);
            content_->AddChildView(tree_view);
        }
        return content_;
    }
};

// 主窗口代理.
class MainWindow : public view::WindowDelegate,
    public view::ButtonListener
{
    enum
    {
        kButtonTitle,
        kButtonAlwaysOnTop,
        kButtonDialog,
        kButtonWidget,
        kButtonClose,
        kButtonCloseWidget,
    };

    view::View* content_;
    bool use_alpha_;
    std::wstring title_;
    bool always_on_top_;

public:
    MainWindow() : content_(NULL), use_alpha_(false),
        title_(L"Hello Window!"), always_on_top_(false)
    {
    }

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
        return title_;
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
            content_->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kVertical,
                0, 0, 0));

            {
                view::TextButton* button = new view::TextButton(this, L"修改窗口标题");
                button->set_tag(kButtonTitle);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"窗口置顶设置");
                button->set_tag(kButtonAlwaysOnTop);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"显示对话框");
                button->set_tag(kButtonDialog);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"创建Widget");
                button->set_tag(kButtonWidget);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"关闭窗口");
                button->set_tag(kButtonClose);
                content_->AddChildView(button);
            }
        }

        return content_;
    }

    virtual void ButtonPressed(view::Button* sender, const view::Event& event)
    {
        switch(sender->tag())
        {
        case kButtonTitle:
            {
                title_ = L"基本的窗体功能演示!";
                window()->UpdateWindowTitle();
            }
            break;
        case kButtonAlwaysOnTop:
            {
                always_on_top_ = !always_on_top_;
                window()->SetIsAlwaysOnTop(always_on_top_);
            }
            break;
        case kButtonDialog:
            {
                ModalDialog* dialog = new ModalDialog();
                view::Window::CreateWanWindow(window()->GetNativeWindow(),
                    gfx::Rect(0, 0, 200, 200), dialog);
                view::CenterAndSizeWindow(window()->GetNativeWindow(),
                    dialog->window()->GetNativeWindow(),
                    dialog->window()->GetBounds().size(), false);
                dialog->window()->Show();
            }
            break;
        case kButtonWidget:
            {
                view::Widget::CreateParams params(view::Widget::CreateParams::TYPE_POPUP);
                params.transparent = true;
                view::Widget* widget = view::Widget::CreateWidget(params);
                gfx::Point point(0, sender->size().height());
                view::View::ConvertPointToScreen(sender, &point);
                gfx::Rect bounds(point.x(), point.y(), 200, 300);
                widget->InitWithWidget(sender->GetWidget(), bounds);

                view::TextButton* close_button = new view::TextButton(this, L"关闭");
                close_button->set_tag(kButtonCloseWidget);
                view::View* widget_container = new view::View();
                widget_container->set_background(view::Background::CreateBackgroundPainter(
                    true, view::Painter::CreateImagePainter(
                    ResourceBundle::GetSharedInstance().GetImageNamed(
                    IDR_TUTORIAL_WIDGET_BACKGROUND), gfx::Insets(6, 10, 6, 10), true)));
                widget_container->SetLayoutManager(new CenterLayout);
                widget_container->AddChildView(close_button);
                widget->SetContentsView(widget_container);
                widget->Show();
            }
            break;
        case kButtonClose:
            {
                window()->CloseWindow();
            }
            break;
        case kButtonCloseWidget:
            sender->GetWidget()->Close();
            break;
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

    view::ViewDelegate::view_delegate = new TestViewsDelegate();
    
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);
    MainWindow* main_window = new MainWindow();
    view::Window::CreateWanWindow(NULL, gfx::Rect(0, 0, 300, 300), main_window);
    view::CenterAndSizeWindow(NULL, main_window->window()->GetNativeWindow(),
        main_window->window()->GetBounds().size(), false);
    main_window->window()->Show();
    MessageLoopForUI::current()->Run(&handler);

    delete view::ViewDelegate::view_delegate;
    view::ViewDelegate::view_delegate = NULL;

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