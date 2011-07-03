
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
#include "view/controls/combobox/combobox.h"
#include "view/controls/combobox/native_combobox_view.h"
#include "view/controls/flash/flash_view.h"
#include "view/controls/focusable_border.h"
#include "view/controls/link.h"
#include "view/controls/listbox/listbox.h"
#include "view/controls/listbox/listbox_model.h"
#include "view/controls/richedit/rich_view.h"
#include "view/controls/scrollbar/bitmap_scroll_bar.h"
#include "view/controls/single_split_view.h"
#include "view/controls/tree/tree_view.h"
#include "view/controls/textfield/textfield.h"
#include "view/controls/web/web_view.h"
#include "view/focus/accelerator_handler.h"
#include "view/layout/box_layout.h"
#include "view/layout/fill_layout.h"
#include "view/painter.h"
#include "view/view.h"
#include "view/view_delegate.h"
#include "view/widget/widget.h"
#include "view/widget/widget_delegate.h"
#include "view/window/dialog_delegate.h"

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

    view::View* TestViewsDelegate::GetDefaultParentView()
    {
        return NULL;
    }

    virtual void AddRef() {}
    virtual void ReleaseRef() {}

    virtual int GetDispositionForEvent(int event_flags) { return 0; }

private:
    mutable scoped_ptr<ui::Clipboard> clipboard_;

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

class ComboboxModelImpl : public ui::ComboboxModel
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
        DCHECK_LT(index, static_cast<int>(arraysize(items_data)));
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
        DCHECK_LT(index, static_cast<int>(arraysize(items_data)));
        return items_data[index];
    }
};

class TreeModelNodeImpl : public ui::TreeModelNode
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

class TreeModelImpl : public ui::TreeModel
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
    virtual ui::TreeModelNode* GetRoot()
    {
        return &root_;
    }

    virtual int GetChildCount(ui::TreeModelNode* parent)
    {
        if(parent == &root_)
        {
            return 5;
        }
        return 0;
    }

    virtual ui::TreeModelNode* GetChild(ui::TreeModelNode* parent, int index)
    {
        if(parent == &root_)
        {
            return &childs_[index];
        }
        return NULL;
    }

    virtual int GetIndexOf(ui::TreeModelNode* parent, ui::TreeModelNode* child)
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

    virtual ui::TreeModelNode* GetParent(ui::TreeModelNode* node)
    {
        if(node != &root_)
        {
            return &root_;
        }

        return NULL;
    }

    virtual void AddObserver(ui::TreeModelObserver* observer)
    {

    }

    virtual void RemoveObserver(ui::TreeModelObserver* observer)
    {

    }

    virtual void SetTitle(ui::TreeModelNode* node, const string16& title)
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

            view::TextButton* text_button = new view::TextButton(NULL, L"按钮");
            text_button->set_focusable(true);
            content_->AddChildView(text_button);

            view::Link* link = new view::Link(L"这是一个链接!");
            content_->AddChildView(link);
        }
        return content_;
    }

    view::Widget* GetWidget()
    {
        return content_->GetWidget();
    }

    const view::Widget* GetWidget() const
    {
        return content_->GetWidget();
    }
};

class WindowDelegate : public view::WidgetDelegate, public view::ScrollBarController
{
    view::View* content_;

public:
    WindowDelegate() : content_(NULL) {}

    virtual std::wstring GetWindowTitle() const
    {
        return L"Widget";
    }

    virtual void DeleteDelegate() { delete this; }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->set_background(
                view::Background::CreateSolidBackground(255, 255, 255));

            view::BitmapScrollBar* vscroll_view = new view::BitmapScrollBar(
                false, true);
            ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
            for(int i=view::BitmapScrollBar::PREV_BUTTON;
                i<view::BitmapScrollBar::THUMB_TRACK; ++i)
            {
                for(int j=view::CustomButton::BS_NORMAL;
                    j<view::CustomButton::BS_DISABLED; ++j)
                {
                    vscroll_view->SetImage(
                        static_cast<view::BitmapScrollBar::ScrollBarPart>(i),
                        static_cast<view::CustomButton::ButtonState>(j),
                        rb.GetBitmapNamed(IDR_VSCROLLBAR_PREV_BUTTON+
                        i*view::CustomButton::BS_DISABLED+j));
                }
            }
            vscroll_view->SetImage(
                view::BitmapScrollBar::THUMB_TRACK,
                view::CustomButton::BS_NORMAL,
                rb.GetBitmapNamed(IDR_VSCROLLBAR_THUMB_TRACK));
            gfx::Size preferred_size = vscroll_view->GetPreferredSize();
            vscroll_view->SetController(this);
            vscroll_view->SetBounds(460, 0, preferred_size.width(), 400);
            vscroll_view->Update(100, 200, 0);
            content_->AddChildView(vscroll_view);

            view::FlashView* flash_view = new view::FlashView(
                L"http://player.youku.com/player.php/sid/XMjc4NDg1Nzgw/v.swf");
            view::WebView* web_view = new view::WebView(L"www.baidu.com");

            view::SingleSplitView* ssv = new view::SingleSplitView(
                flash_view, web_view,
                view::SingleSplitView::VERTICAL_SPLIT, NULL);
            ssv->SetBounds(0, 0, 450, 800);
            content_->AddChildView(ssv);
        }
        return content_;
    }

    view::Widget* GetWidget()
    {
        return content_->GetWidget();
    }

    const view::Widget* GetWidget() const
    {
        return content_->GetWidget();
    }

    // view::ScrollBarController实现
    virtual void ScrollToPosition(view::ScrollBar* source, int position) {}

    virtual int GetScrollIncrement(view::ScrollBar* source, bool is_page,
        bool is_positive)
    {
        return 10;
    }
};

// 主窗口代理.
class MainWindow : public view::WidgetDelegate,
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
    view::RichView* rich_view1_;
    view::RichView* rich_view2_;
    std::wstring title_;
    bool always_on_top_;

public:
    MainWindow() : content_(NULL),
        rich_view1_(NULL), rich_view2_(NULL),
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
                view::TextButton* button = new view::TextButton(this, L"显示Widget");
                button->set_tag(kButtonWidget);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"关闭窗口");
                button->set_tag(kButtonClose);
                content_->AddChildView(button);
            }
            {
                view::Checkbox* checkbox = new view::Checkbox(L"多选框");
                content_->AddChildView(checkbox);
            }
            {
                view::RadioButton* radio_button = new view::RadioButton(L"单选", 1);
                content_->AddChildView(radio_button);
            }

            {
                rich_view1_ = new view::RichView(
                    ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL);
                rich_view2_ = new view::RichView(
                    ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL);
                view::SingleSplitView* ssv = new view::SingleSplitView(
                    rich_view1_, rich_view2_,
                    view::SingleSplitView::HORIZONTAL_SPLIT, NULL);
                rich_view1_->set_background(
                    view::Background::CreateStandardPanelBackground());
                rich_view1_->set_border(new view::FocusableBorder());
                rich_view2_->set_background(
                    view::Background::CreateStandardPanelBackground());
                rich_view2_->set_border(new view::FocusableBorder());
                content_->AddChildView(ssv);

                view::RichView* rich_view = new view::RichView(
                    ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL);
                rich_view->set_border(new view::FocusableBorder());
                content_->AddChildView(rich_view);
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
                GetWidget()->UpdateWindowTitle();
            }
            break;
        case kButtonAlwaysOnTop:
            {
                always_on_top_ = !always_on_top_;
                GetWidget()->SetAlwaysOnTop(always_on_top_);
            }
            break;
        case kButtonDialog:
            {
                ModalDialog* dialog = new ModalDialog();
                view::Widget::CreateWindowWithParentAndBounds(dialog,
                    GetWidget()->GetNativeWindow(),
                    gfx::Rect(0, 0, 240, 300));
                ui::CenterAndSizeWindow(GetWidget()->GetNativeWindow(),
                    dialog->GetWidget()->GetNativeWindow(),
                    dialog->GetWidget()->GetWindowScreenBounds().size(), false);
                dialog->GetWidget()->Show();
            }
            break;
        case kButtonWidget:
            {
                WindowDelegate* widget = new WindowDelegate();
                view::Widget::CreateWindowWithParentAndBounds(widget,
                    GetWidget()->GetNativeWindow(),
                    gfx::Rect(0, 0, 500, 800));
                ui::CenterAndSizeWindow(GetWidget()->GetNativeWindow(),
                    widget->GetWidget()->GetNativeWindow(),
                    widget->GetWidget()->GetWindowScreenBounds().size(), false);
                widget->GetWidget()->Show();
            }
            break;
        case kButtonClose:
            {
                GetWidget()->Close();
            }
            break;
        case kButtonCloseWidget:
            sender->GetWidget()->Close();
            break;
        }
    }

    void Init()
    {
        if(rich_view1_)
        {
            rich_view1_->SetText(L"编辑框一");
        }
        if(rich_view2_)
        {
            rich_view2_->SetText(L"编辑框二");
        }
    }

    view::Widget* GetWidget()
    {
        return content_->GetWidget();
    }

    const view::Widget* GetWidget() const
    {
        return content_->GetWidget();
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
    CommandLine::Init(0, NULL);

    FilePath res_dll;
    PathProvider(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    view::ViewDelegate::view_delegate = new TestViewsDelegate();
    
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);
    MainWindow* main_window = new MainWindow();
    view::Widget::CreateWindow(main_window);
    ui::CenterAndSizeWindow(NULL,
        main_window->GetWidget()->GetNativeWindow(),
        gfx::Size(400, 400), false);
    main_window->GetWidget()->Show();
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