
#include "view_prop.h"

#include <set>

namespace view
{

    // 维护实际的视图, 键和数据.
    class ViewProp::Data : public base::RefCounted<ViewProp::Data>
    {
    public:
        // 返回视图/键对应的Data*. 如果|create|为false, 且尚未调用过|Get|, 返回NULL.
        static void Get(HWND view, const char* key, bool create,
            scoped_refptr<Data>* data)
        {
            if(!data_set_)
            {
                data_set_ = new DataSet;
            }
            scoped_refptr<Data> new_data(new Data(view, key));
            DataSet::const_iterator i = data_set_->find(new_data.get());
            if(i != data_set_->end())
            {
                *data = *i;
                return;
            }
            if(!create)
            {
                return;
            }
            data_set_->insert(new_data.get());
            *data = new_data.get();
        }

        // 数据.
        void set_data(void* data) { data_ = data; }
        void* data() const { return data_; }

        const char* key() const { return key_; }

    private:
        friend class base::RefCounted<Data>;

        // 用于数据在map中排序.
        class DataComparator
        {
        public:
            bool operator()(const Data* d1, const Data* d2) const
            {
                return (d1->view_==d2->view_) ? (d1->key_<d2->key_) :
                    (d1->view_<d2->view_);
            }
        };

        typedef std::set<Data*, DataComparator> DataSet;

        Data(HWND view, const char* key) : view_(view),
            key_(key), data_(NULL) {}

        ~Data()
        {
            DataSet::iterator i = data_set_->find(this);
            // 使用==判断相当, 因为|Get|会创建无用的值.
            if(i!=data_set_->end() && *i==this)
            {
                data_set_->erase(i);
            }
        }

        // Data集合存储在这里. ~Data会从集合中移除.
        static DataSet* data_set_;

        const HWND view_;
        const char* key_;
        void* data_;

        DISALLOW_COPY_AND_ASSIGN(Data);
    };

    // static
    ViewProp::Data::DataSet* ViewProp::Data::data_set_ = NULL;

    ViewProp::ViewProp(HWND view, const char* key, void* data)
    {
        Data::Get(view, key, true, &data_);
        data_->set_data(data);
    }

    ViewProp::~ViewProp()
    {
        // 用于提供类似SetProp的语义. ~ViewProp的行为应该和调用RemoveProp类似.
        data_->set_data(NULL);
    }

    // static
    void* ViewProp::GetValue(HWND view, const char* key)
    {
        scoped_refptr<Data> data;
        Data::Get(view, key, false, &data);
        return data.get() ? data->data() : NULL;
    }

    // static
    const char* ViewProp::Key() const
    {
        return data_->key();
    }

} //namespace view