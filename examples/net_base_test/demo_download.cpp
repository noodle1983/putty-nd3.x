
#include "demo_download.h"

#include "view/controls/button/text_button.h"
#include "view/layout/box_layout.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

DemoDownload::DemoDownload(DemoMain* main) : DemoBase(main),
download_engine_(NULL), start_stop_(NULL) {}

DemoDownload::~DemoDownload()
{
    if(download_engine_)
    {
        download_engine_->Shutdown();
        download_engine_ = NULL;
    }
}

std::wstring DemoDownload::GetDemoTitle()
{
    return std::wstring(L"Download");
}

void DemoDownload::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL, 1,
        view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(0, 0);
    view::View* toolbar_container_ = new view::View();
    toolbar_container_->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kHorizontal, 0, 0, 0));
    layout->AddView(toolbar_container_);

    start_stop_ = new view::TextButton(this, L"启动下载引擎");
    toolbar_container_->AddChildView(start_stop_);
}

void DemoDownload::ButtonPressed(view::Button* sender,
                                 const view::Event& event)
{
    if(sender == start_stop_)
    {
        if(download_engine_)
        {
            download_engine_->Shutdown();
            download_engine_ = NULL;
        }
        else
        {
            download_engine_ = StartDownloadEngine();
            wchar_t** urls = new wchar_t*[1];
            urls[0] = new wchar_t[2048];
            wcscpy(urls[0], L"http://dl_dir.qq.com/invc/qqimage/QQImage_Setup_14_295.exe");
            download_engine_->AddTask(0, urls, 1);
            delete urls[0];
            delete[] urls;
        }

        start_stop_->SetText(download_engine_ ? L"关闭下载引擎" :
            L"启动下载引擎");
    }
}