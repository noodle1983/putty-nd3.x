
#ifndef __ui_base_multi_animation_h__
#define __ui_base_multi_animation_h__

#pragma once

#include <vector>

#include "animation.h"
#include "tween.h"

namespace ui
{

    // MultiAnimation是组合动画, 包括一些子动画的动画. 传递parts创建MultiAnimation,
    // 调用Start(), 动画步进的时候会通知代理. MultiAnimation缺省一直运行直到调用Stop,
    // 更多细节参见|set_continuous()|.
    class MultiAnimation : public Animation
    {
    public:
        // 部分动画的定义. 每个部分都由下面这些组成:
        //
        // time_ms: 这部分的时间.
        // start_time_ms: 用于计算动画起始位置的百分比.
        // end_time_ms: 用于计算动画结束位置的百分比.
        //
        // 大多数情况下, |start_time_ms|=0, |end_time_ms|=|time_ms|. 如果想要得到不同
        // 的效果, 可以调整start/end. 比如执行一个200ms动画的[0.25, 0.75]部分, 使用这
        // 三个值: 200、100、400.
        struct Part
        {
            Part() : time_ms(0), start_time_ms(0),
                end_time_ms(0), type(Tween::ZERO) {}
            Part(int time_ms, Tween::Type type) : time_ms(time_ms),
                start_time_ms(0), end_time_ms(time_ms), type(type) {}

            int time_ms;
            int start_time_ms;
            int end_time_ms;
            Tween::Type type;
        };

        typedef std::vector<Part> Parts;

        explicit MultiAnimation(const Parts& parts);
        virtual ~MultiAnimation();

        // 设置动画结束时是否继续执行. 如果为true, 动画一直运行直到显式调用停止.
        // 缺省值为true.
        void set_continuous(bool continuous) { continuous_ = continuous; }

        // 返回当前值. MultiAnimation的当前值由当前部分动画的补间类型确定.
        virtual double GetCurrentValue() const { return current_value_; }

        // 返回当前部分动画的索引.
        size_t current_part_index() const { return current_part_index_; }

    protected:
        virtual void Step(base::TimeTicks time_now);
        virtual void SetStartTime(base::TimeTicks start_time);

    private:
        // 返回给定时间的部分动画. |time_ms|会被重置为相对这个部分动画的时间,
        // |part_index|是这个部分动画的索引.
        const Part& GetPart(int* time_ms, size_t* part_index);

        // 整个动画的组成部分.
        const Parts parts_;

        // 所有部分动画的总时间.
        const int cycle_time_ms_;

        // 动画的当前值.
        double current_value_;

        // 当前部分动画的索引.
        size_t current_part_index_;

        // 参见上面设置函数处的描述.
        bool continuous_;

        DISALLOW_COPY_AND_ASSIGN(MultiAnimation);
    };

} //namespace ui

#endif //__ui_base_multi_animation_h__