
#ifndef __base_raw_scoped_refptr_mismatch_checker_h__
#define __base_raw_scoped_refptr_mismatch_checker_h__

#pragma once

#include "base/template_util.h"
#include "base/tuple.h"
#include "ref_counted.h"

// 给函数的scoped_refptr<>类型参数传递一个原始指针的任务是非常危险的. 编译器会
// 接收这种情况, 但任务被执行前不会增加引用计数. 如果调用者希望在传递参数的时候
// 就增加引用计数, 结果会让他大吃一惊! 比如: http://crbug.com/27191.
// 下面一组特性被设计用来产生编译时错误, 阻止这种事情的发生.

namespace base
{

    // 只在task.h和callback.h中使用. 不提供公共使用, 所以封装在internal命名空间.
    namespace internal
    {

        template<typename T>
        struct NeedsScopedRefptrButGetsRawPtr
        {
            enum
            {
                value = base::false_type::value
            };
        };

        template<typename Params>
        struct ParamsUseScopedRefptrCorrectly
        {
            enum { value = 0 };
        };

        template<>
        struct ParamsUseScopedRefptrCorrectly<Tuple0>
        {
            enum { value = 1 };
        };

        template<typename A>
        struct ParamsUseScopedRefptrCorrectly<Tuple1<A> >
        {
            enum { value = !NeedsScopedRefptrButGetsRawPtr<A>::value };
        };

        template<typename A, typename B>
        struct ParamsUseScopedRefptrCorrectly<Tuple2<A, B> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value) };
        };

        template<typename A, typename B, typename C>
        struct ParamsUseScopedRefptrCorrectly<Tuple3<A, B, C> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value) };
        };

        template<typename A, typename B, typename C, typename D>
        struct ParamsUseScopedRefptrCorrectly<Tuple4<A, B, C, D> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value ||
                NeedsScopedRefptrButGetsRawPtr<D>::value) };
        };

        template<typename A, typename B, typename C, typename D, typename E>
        struct ParamsUseScopedRefptrCorrectly<Tuple5<A, B, C, D, E> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value ||
                NeedsScopedRefptrButGetsRawPtr<D>::value ||
                NeedsScopedRefptrButGetsRawPtr<E>::value) };
        };

        template<typename A, typename B, typename C, typename D, typename E,
            typename F>
        struct ParamsUseScopedRefptrCorrectly<Tuple6<A, B, C, D, E, F> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value ||
                NeedsScopedRefptrButGetsRawPtr<D>::value ||
                NeedsScopedRefptrButGetsRawPtr<E>::value ||
                NeedsScopedRefptrButGetsRawPtr<F>::value) };
        };

        template<typename A, typename B, typename C, typename D, typename E,
            typename F, typename G>
        struct ParamsUseScopedRefptrCorrectly<Tuple7<A, B, C, D, E, F, G> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value ||
                NeedsScopedRefptrButGetsRawPtr<D>::value ||
                NeedsScopedRefptrButGetsRawPtr<E>::value ||
                NeedsScopedRefptrButGetsRawPtr<F>::value ||
                NeedsScopedRefptrButGetsRawPtr<G>::value) };
        };

        template<typename A, typename B, typename C, typename D, typename E,
            typename F, typename G, typename H>
        struct ParamsUseScopedRefptrCorrectly<Tuple8<A, B, C, D, E, F, G, H> >
        {
            enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                NeedsScopedRefptrButGetsRawPtr<B>::value ||
                NeedsScopedRefptrButGetsRawPtr<C>::value ||
                NeedsScopedRefptrButGetsRawPtr<D>::value ||
                NeedsScopedRefptrButGetsRawPtr<E>::value ||
                NeedsScopedRefptrButGetsRawPtr<F>::value ||
                NeedsScopedRefptrButGetsRawPtr<G>::value ||
                NeedsScopedRefptrButGetsRawPtr<H>::value) };
        };

    } //namespace internal

} //namespace base

#endif //__base_raw_scoped_refptr_mismatch_checker_h__