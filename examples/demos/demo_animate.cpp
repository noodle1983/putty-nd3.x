
#include "demo_animate.h"

#include "ui_base/resource/resource_bundle.h"

#include "view/animation/bounds_animator.h"
#include "view/controls/button/text_button.h"
#include "view/controls/image_view.h"
#include "view/controls/label.h"
#include "view/layout/box_layout.h"
#include "view/layout/grid_layout.h"

#include "../wanui_res/resource.h"

#include "demo_main.h"

class AppView : public view::View
{
public:
    AppView(int res_id, const std::wstring& app_name);
    virtual ~AppView();

    void SaveBounds() { save_bounds_ = bounds(); }
    gfx::Rect GetSaveBounds() const { return save_bounds_; }

private:
    gfx::Rect save_bounds_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(AppView);
};

AppView::AppView(int res_id, const std::wstring& app_name)
{
    view::GridLayout* layout = new view::GridLayout(this);
    SetLayoutManager(layout);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.0f, view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(0.0f, 0);
    view::ImageView* image = new view::ImageView();
    image->SetImage(
        ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(res_id));
    layout->AddView(image);

    layout->StartRow(0.0f, 0);
    view::Label* label = new view::Label(app_name);
    layout->AddView(label);
}

AppView::~AppView()
{
}

class AppLayout : public view::LayoutManager
{
public:
    AppLayout(int inside_border_horizontal_spacing,
        int inside_border_vertical_spacing,
        int between_child_spacing);

    // Overridden from view::LayoutManager:
    virtual gfx::Size GetPreferredSize(view::View* host)
    {
        return gfx::Size();
    }
    virtual void Layout(view::View* host);

private:
    // Spacing between child views and host view border.
    const int inside_border_horizontal_spacing_;
    const int inside_border_vertical_spacing_;

    // Spacing to put in between child views.
    const int between_child_spacing_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(AppLayout);
};

AppLayout::AppLayout(int inside_border_horizontal_spacing,
                     int inside_border_vertical_spacing,
                     int between_child_spacing)
                     : inside_border_horizontal_spacing_(inside_border_horizontal_spacing),
                     inside_border_vertical_spacing_(inside_border_vertical_spacing),
                     between_child_spacing_(between_child_spacing) {}

void AppLayout::Layout(view::View* host)
{
    gfx::Rect childArea(gfx::Rect(host->size()));
    childArea.Inset(host->GetInsets());
    childArea.Inset(inside_border_horizontal_spacing_,
        inside_border_vertical_spacing_);
    int x = childArea.x();
    int y = childArea.y();
    for(int i=0; i<host->child_count(); ++i)
    {
        view::View* child = host->child_at(i);
        if(child->IsVisible())
        {
            gfx::Size size(child->GetPreferredSize());
            gfx::Rect bounds(x, y, 0, size.height());
            if(x + size.width() <= childArea.right())
            {
                bounds.set_width(size.width());
                x += size.width() + between_child_spacing_;
            }
            else
            {
                x = childArea.x();
                y += size.height() + between_child_spacing_;
                bounds.SetRect(x, y, size.width(), size.height());

                x += size.width() + between_child_spacing_;
            }

            // Clamp child view bounds to |childArea|.
            child->SetBoundsRect(bounds.Intersect(childArea));
        }
    }
}

struct AppInfo 
{
    int icon_res_id;
    std::wstring app_name;
};
namespace
{
    const AppInfo app_infos[] =
    {
        { IDR_APP_1,  L"app1"  },
        { IDR_APP_2,  L"app2"  },
        { IDR_APP_3,  L"app3"  },
        { IDR_APP_4,  L"app4"  },
        { IDR_APP_5,  L"app5"  },
        { IDR_APP_6,  L"app6"  },
        { IDR_APP_7,  L"app7"  },
        { IDR_APP_8,  L"app8"  },
        { IDR_APP_9,  L"app9"  },
        { IDR_APP_10, L"app10" },
        { IDR_APP_11, L"app11" },
        { IDR_APP_12, L"app12" },
        { IDR_APP_13, L"app13" },
        { IDR_APP_14, L"app14" },
        { IDR_APP_15, L"app15" },
        { IDR_APP_16, L"app16" },
        { IDR_APP_17, L"app17" },
        { IDR_APP_18, L"app18" },
        { IDR_APP_19, L"app19" },
        { IDR_APP_20, L"app20" },
        { IDR_APP_21, L"app21" },
        { IDR_APP_22, L"app22" },
        { IDR_APP_23, L"app23" }
    };
}

class AppContainer : public view::View, public view::BoundsAnimatorObserver
{
public:
    AppContainer();
    virtual ~AppContainer();

    void DoAnimate();

    // Overridden from view::View:
    virtual void Layout()
    {
        view::View::Layout();

        for(int i=0; i<child_count(); ++i)
        {
            AppView* child = static_cast<AppView*>(child_at(i));
            child->SaveBounds();
        }
    }

    // Overridden from view::BoundsAnimatorObserver:
    virtual void OnBoundsAnimatorDone(view::BoundsAnimator* animator)
    {
        DCHECK(animator == &bounds_animator_);
        if(show_)
        {
            for(int i=0; i<child_count(); ++i)
            {
                AppView* child = static_cast<AppView*>(child_at(i));
                child->SaveBounds();
            }
        }
    }

private:
    view::BoundsAnimator bounds_animator_;
    bool show_;

    DISALLOW_COPY_AND_ASSIGN(AppContainer);
};

AppContainer::AppContainer() : bounds_animator_(this), show_(true)
{
    bounds_animator_.set_observer(this);

    SetLayoutManager(new AppLayout(10, 10, 10));
    for(int i=0; i<arraysize(app_infos); ++i)
    {
        AddChildView(new AppView(app_infos[i].icon_res_id,
            app_infos[i].app_name));
    }
}

AppContainer::~AppContainer()
{
    bounds_animator_.Cancel();
}

void AppContainer::DoAnimate()
{
    show_ = !show_;
    for(int i=0; i<child_count(); ++i)
    {
        AppView* child = static_cast<AppView*>(child_at(i));
        if(show_)
        {
            bounds_animator_.AnimateViewTo(child, child->GetSaveBounds());
        }
        else
        {
            gfx::Point target_orign(-child->width(), -child->height());
            gfx::Rect layout_rect = child->bounds();
            gfx::Point app_view_center = layout_rect.CenterPoint();
            gfx::Point view_center = GetLocalBounds().CenterPoint();
            if(app_view_center.x() >= view_center.x() &&
                app_view_center.y() <= view_center.y())
            {
                target_orign.set_x(GetLocalBounds().right());
            }
            else if(app_view_center.x() <= view_center.x() &&
                app_view_center.y() >= view_center.y())
            {
                target_orign.set_y(GetLocalBounds().bottom());
            }
            else if(app_view_center.x() >= view_center.x() &&
                app_view_center.y() >= view_center.y())
            {
                target_orign.set_x(GetLocalBounds().right());
                target_orign.set_y(GetLocalBounds().bottom());
            }
            bounds_animator_.AnimateViewTo(child,
                gfx::Rect(target_orign, child->size()));
        }
    }
}


DemoAnimate::DemoAnimate(DemoMain* main) : DemoBase(main),
apps_container_(NULL), button_animate_(NULL)  {}

DemoAnimate::~DemoAnimate() {}

std::wstring DemoAnimate::GetDemoTitle()
{
    return std::wstring(L"Animate");
}

void DemoAnimate::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    apps_container_ = new AppContainer();
    button_animate_ = new view::NativeTextButton(this, L"ÑÝÊ¾¶¯»­");

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(1.0f, 0);
    layout->AddView(apps_container_);

    layout->StartRow(0.0f, 0);
    layout->AddView(button_animate_);
}

void DemoAnimate::ButtonPressed(view::Button* sender,
                                const view::Event& event)
{
    if(button_animate_ == sender)
    {
        (static_cast<AppContainer*>(apps_container_))->DoAnimate();
    }
}