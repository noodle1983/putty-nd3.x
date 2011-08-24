
#include "demo_base.h"

#include "view/view.h"
#include "demo_main.h"

namespace
{

    class ContainerView : public view::View
    {
    public:
        explicit ContainerView(DemoBase* base)
            : demo_view_created_(false),
            demo_base_(base) {}

    protected:
        // views::View overrides:
        virtual void ViewHierarchyChanged(bool is_add,
            view::View* parent, view::View* child)
        {
            view::View::ViewHierarchyChanged(is_add, parent, child);
            if(is_add && GetWidget() && !demo_view_created_)
            {
                demo_view_created_ = true;
                demo_base_->CreateDemoView(this);
            }
        }

    private:
        bool demo_view_created_;

        DemoBase* demo_base_;

        DISALLOW_COPY_AND_ASSIGN(ContainerView);
    };

}

DemoBase::DemoBase(DemoMain* main) : main_(main)
{
    container_ = new ContainerView(this);
}

void DemoBase::PrintStatus(const std::wstring& status)
{
    main_->SetStatus(status);
}