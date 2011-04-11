
#ifndef __base_template_util_h__
#define __base_template_util_h__

#pragma once

namespace base
{

    // 来自tr1的模板.

    template<class T, T v>
    struct integral_constant
    {
        static const T value = v;
        typedef T value_type;
        typedef integral_constant<T, v> type;
    };

    template<class T, T v> const T integral_constant<T, v>::value;

    typedef integral_constant<bool, true> true_type;
    typedef integral_constant<bool, false> false_type;

    template<class T> struct is_pointer : false_type {};
    template<class T> struct is_pointer<T*> : true_type {};

    template<class> struct is_array : public false_type {};
    template<class T, size_t n> struct is_array<T[n]> : public true_type {};
    template<class T> struct is_array<T[]> : public true_type {};

    template<class T> struct is_non_const_reference : false_type {};
    template<class T> struct is_non_const_reference<T&> : true_type {};
    template<class T> struct is_non_const_reference<const T&> : false_type {};

    namespace internal
    {

        // 类型YesType和NoType确保sizeof(YesType) < sizeof(NoType).
        typedef char YesType;

        struct NoType
        {
            YesType dummy[2];
        };

        // ConvertHelper实现了is_convertible, 你无需关心它是怎么工作的. 对于
        // 希望知道细节的人: 我们声明了两个不同的函数, 一个参数类型为To, 另
        // 一个带有变成的参数列表. 它们的返回值大小不相同, 这样我们可以使用
        // From类型的参数调用, 通过sizeof得知编译器选择的版本. 这类技巧的更多
        // 细节参见Alexandrescu的_Modern C++ Design_.
        struct ConvertHelper
        {
            template<typename To>
            static YesType Test(To);

            template<typename To>
            static NoType Test(...);

            template<typename From>
            static From Create();
        };

        // 用于确定类型是否为struct/union/class. 源于Boost的is_class的type_trait
        // 实现.
        struct IsClassHelper
        {
            template<typename C>
            static YesType Test(void(C::*)(void));

            template<typename C>
            static NoType Test(...);
        };

    } //namespace internal

    // 如果From可以转化为To, 从true_type继承, 否则从false_type继承.
    //
    // 注意如果类型是可转化的, 不管转换是否会产生警告都会从true_type继承.
    template<typename From, typename To>
    struct is_convertible
        : integral_constant<bool,
        sizeof(internal::ConvertHelper::Test<To>(
        internal::ConvertHelper::Create<From>())) ==
        sizeof(internal::YesType)> {};

    template<typename T>
    struct is_class
        : integral_constant<bool,
        sizeof(internal::IsClassHelper::Test<T>(0)) ==
        sizeof(internal::YesType)> {};

} //namespace base

#endif //__base_template_util_h__