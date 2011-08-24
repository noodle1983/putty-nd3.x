
#ifndef __base_scoped_comptr_h__
#define __base_scoped_comptr_h__

#pragma once

#include <unknwn.h>

#include "base/logging.h"
#include "base/memory/ref_counted.h"

namespace base
{
    namespace win
    {

        // 模拟COM接口的智能指针. 使用scoped_refptr作为积累提供引用计数功能,
        // 添加了IUnknown的一些特定服务.
        template<class Interface, const IID* interface_id=&__uuidof(Interface)>
        class ScopedComPtr : public scoped_refptr<Interface>
        {
        public:
            // 工具模板, 阻止使用者透过ScopedComPtr类调用接口指针的AddRef和(或)
            // Release.
            class BlockIUnknownMethods : public Interface
            {
            private:
                STDMETHOD(QueryInterface)(REFIID iid, void** object) = 0;
                STDMETHOD_(ULONG, AddRef)() = 0;
                STDMETHOD_(ULONG, Release)() = 0;
            };

            typedef scoped_refptr<Interface> ParentClass;

            ScopedComPtr() {}

            explicit ScopedComPtr(Interface* p) : ParentClass(p) {}

            ScopedComPtr(const ScopedComPtr<Interface, interface_id>& p)
                : ParentClass(p) {}

            ~ScopedComPtr()
            {
                // 智能指针类的大小跟裸指针保持一致.
                COMPILE_ASSERT(sizeof(ScopedComPtr<Interface, interface_id>) ==
                    sizeof(Interface*), ScopedComPtrSize);
            }

            // 显式调用内部对象的Release()方法. 为ScopedComPtr实例提供重用功能.
            // 函数与IUnknown::Release等价, 不要与scoped_ptr::release()混淆.
            void Release()
            {
                if(ptr_ != NULL)
                {
                    ptr_->Release();
                    ptr_ = NULL;
                }
            }

            // 使内部对象与本对象分离并返回其指针.
            Interface* Detach()
            {
                Interface* p = ptr_;
                ptr_ = NULL;
                return p;
            }

            // 接受一个已经被addref的接口指针.
            void Attach(Interface* p)
            {
                DCHECK(ptr_ == NULL);
                ptr_ = p;
            }

            // 返回接口指针的地址.
            // 用于接受输出参数(接管所有权). 函数使用DCHECKs验证当前值是否为NULL.
            // 用法: Foo(p.Receive());
            Interface** Receive()
            {
                DCHECK(ptr_==NULL) << "Object leak. Pointer must be NULL";
                return &ptr_;
            }

            // 返回void**类型的接口指针的地址.
            void** ReceiveVoid()
            {
                return reinterpret_cast<void**>(Receive());
            }

            template<class Query>
            HRESULT QueryInterface(Query** p)
            {
                DCHECK(p != NULL);
                DCHECK(ptr_ != NULL);
                // IUnknown已经有一个模板版本的QueryInterface, 所以iid参数在
                // 这里是隐式的. 这里只是添加了DCHECKs.
                return ptr_->QueryInterface(p);
            }

            // 在IID没与类型关联时QI.
            HRESULT QueryInterface(const IID& iid, void** obj)
            {
                DCHECK(obj != NULL);
                DCHECK(ptr_ != NULL);
                return ptr_->QueryInterface(iid, obj);
            }

            // 从|other|查询内部对象接口, 并返回other->QueryInterface操作的错误码.
            HRESULT QueryFrom(IUnknown* object)
            {
                DCHECK(object != NULL);
                return object->QueryInterface(Receive());
            }

            // CoCreateInstance封装.
            HRESULT CreateInstance(const CLSID& clsid, IUnknown* outer=NULL,
                DWORD context=CLSCTX_ALL)
            {
                DCHECK(ptr_ == NULL);
                HRESULT hr = ::CoCreateInstance(clsid, outer, context,
                    *interface_id, reinterpret_cast<void**>(&ptr_));
                return hr;
            }

            // 检查本对象与|other|是否相同.
            bool IsSameObject(IUnknown* other)
            {
                if(!other && !ptr_)
                {
                    return true;
                }

                if(!other || !ptr_)
                {
                    return false;
                }

                ScopedComPtr<IUnknown> my_identity;
                QueryInterface(my_identity.Receive());

                ScopedComPtr<IUnknown> other_identity;
                other->QueryInterface(other_identity.Receive());

                return static_cast<IUnknown*>(my_identity) ==
                    static_cast<IUnknown*>(other_identity);
            }

            // 提供接口的直接访问. 这里使用了一个常用的技巧限制访问IUknown的
            // 方法, 以免发生类似下面的错误做法:
            //     ScopedComPtr<IUnknown> p(Foo());
            //     p->Release();
            //     ... 后面析构函数执行时, 会再次调用Release().
            // 还能得益于QueryInterface添加的DCHECKs. 假如你静态的强转ScopedComPtr到
            // 被封装的接口, 你还是可以通过接口指针访问这些方法, 但是一般不要那样做.
            BlockIUnknownMethods* operator->() const
            {
                DCHECK(ptr_ != NULL);
                return reinterpret_cast<BlockIUnknownMethods*>(ptr_);
            }

            // 使用父类的operator=().
            using scoped_refptr<Interface>::operator=;

            static const IID& iid()
            {
                return *interface_id;
            }
        };

    } //namespace win
} //namespace base

#endif //__base_scoped_comptr_h__