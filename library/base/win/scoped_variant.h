
#ifndef __base_win_scoped_variant_h__
#define __base_win_scoped_variant_h__

#pragma once

#include <windows.h>
#include <oleauto.h>

#include "../basic_types.h"

namespace base
{

    // ScopedVariant类在退出作用域时自动释放COM VARIANT. 除此之外还提供了一些
    // 方便使用VARIANT的封装函数.
    // 采用包容而不是派生的方式, 这样有更多的控制权, 避免内存泄露.
    class ScopedVariant
    {
    public:
        // 声明一个全局的VT_EMPTY类型的variant变量.
        static const VARIANT kEmptyVariant;

        // 缺省构造函数.
        ScopedVariant()
        {
            // 等价于VariantInit, 但是代码更少.
            var_.vt = VT_EMPTY;
        }

        // 构造函数, 创建VT_BSTR类型的VARIANT.
        // 注意: 传递的BSTR所有权不会被接管.
        explicit ScopedVariant(const wchar_t* str);

        // 创建一个指定长度的VT_BSTR类型的VARIANT.
        explicit ScopedVariant(const wchar_t* str, UINT length);

        // 创建一个整数类型的VARIANT, 赋值到VARIANT.lVal(长度32bit).
        explicit ScopedVariant(int value, VARTYPE vt=VT_I4);

        // 创建一个double精度类型的VARIANT, |vt|要么是VT_R8要么是VT_DATE.
        explicit ScopedVariant(double value, VARTYPE vt=VT_R8);

        // VT_DISPATCH.
        explicit ScopedVariant(IDispatch* dispatch);

        // VT_UNKNOWN.
        explicit ScopedVariant(IUnknown* unknown);

        // SAFEARRAY.
        explicit ScopedVariant(SAFEARRAY* safearray);

        // 拷贝构造函数.
        explicit ScopedVariant(const VARIANT& var);

        ~ScopedVariant();

        inline VARTYPE type() const
        {
            return var_.vt;
        }

        // 重新接管一个以分配的VARIANT.
        void Reset(const VARIANT& var=kEmptyVariant);

        // 释放VARIANT的所有权给调用者.
        VARIANT Release();

        // 交换两个ScopedVariant.
        void Swap(ScopedVariant& var);

        // 返回一份VARIANT拷贝.
        VARIANT Copy() const;

        // 返回0表示相等, 1表示大于|var|, -1表示小于|var|.
        int Compare(const VARIANT& var, bool ignore_case=false) const;

        // 返回指针地址.
        // 用作接受VARIANT类型的输出参数(接管所有权). 函数会DCHECKs当前值为
        // empty/null. 用法: GetVariant(var.receive());
        VARIANT* Receive();

        void Set(const wchar_t* str);

        void Set(int8 i8);
        void Set(uint8 ui8);
        void Set(int16 i16);
        void Set(uint16 ui16);
        void Set(int32 i32);
        void Set(uint32 ui32);
        void Set(int64 i64);
        void Set(uint64 ui64);
        void Set(float r32);
        void Set(double r64);
        void Set(bool b);

        // 创建|var|的拷贝作为对象的值. 注意这不同于Reset()方法, Reset()
        // 会释放当前对象并接管所有权.
        void Set(const VARIANT& var);

        // 支持COM对象.
        void Set(IDispatch* disp);
        void Set(IUnknown* unk);

        // 支持SAFEARRAY.
        void Set(SAFEARRAY* array);

        // 支持DATE, DATE是double. 已经有一个Set(double)了.
        void SetDate(DATE date);

        // 允许常量方式访问内部的variant, 不会DCHECKs. 这样做是为了支持V_XYZ
        // (比如V_BSTR)系列宏, 但是不允许修改, 因为我们有variant的控制权.
        const VARIANT* operator&() const
        {
            return &var_;
        }

        // 像其它作用域类(scoped_refptr、ScopedComPtr、ScopedBstr)一样支持赋值操作符.
        ScopedVariant& operator=(const VARIANT& var);

        // 传递指针给一个输入只读类型的函数, 但是函数原型中是非const的指针.
        // 这里不需要DCHECK. 调用者需要清楚用法.
        VARIANT* AsInput() const
        {
            return const_cast<VARIANT*>(&var_);
        }

        // 允许ScopedVariant实例按值或者按常量引用传递.
        operator const VARIANT&() const
        {
            return var_;
        }

        // 用于调试时检测是否有内存泄漏.
        static bool IsLeakableVarType(VARTYPE vt);

    protected:
        VARIANT var_;

    private:
        // 不支持ScopedVariant的比较操作. 使用Compare方法替代.
        bool operator==(const ScopedVariant& var) const;
        bool operator!=(const ScopedVariant& var) const;
        DISALLOW_COPY_AND_ASSIGN(ScopedVariant);
    };

} //namespace base

#endif //__base_win_scoped_variant_h__