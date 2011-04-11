
#ifndef __base_time_h__
#define __base_time_h__

#pragma once

#include <time.h>
#include <windows.h>

#include "basic_types.h"

// Time表示一个绝对的时间点, 由于依赖平台的起始时间, 内部用微秒
// (s/1,000,000)表示.
//
// TimeDelta表示一段时间, 内部用微秒表示.
//
// TimeTicks表示一个递增的抽象时间, 用于计算时间差, 内部用微秒表示.
// 它无法转换成可读的时间, 确保不会减小.(用户修改时间, Time::Now()
// 可能会减小或者猛增).
//
// 由于内部用64位数值表示, 所以按值传递是高效的.

namespace base
{

    class Time;
    class TimeTicks;

    class TimeDelta
    {
    public:
        TimeDelta() : delta_(0) {}

        // 转换时间到TimeDelta.
        static TimeDelta FromDays(int64 days);
        static TimeDelta FromHours(int64 hours);
        static TimeDelta FromMinutes(int64 minutes);
        static TimeDelta FromSeconds(int64 secs);
        static TimeDelta FromMilliseconds(int64 ms);
        static TimeDelta FromMicroseconds(int64 us);

        // 返回TimeDelta对象的内部数值. 不要直接做算术运算, 应该使用提供的操作函数.
        int64 ToInternalValue() const
        {
            return delta_;
        }

        // 返回TimeDelta的一些单位. F版本返回浮点类型, 其它的返回取整后的数值.
        //
        // InMillisecondsRoundedUp()取整到下一微秒.
        int InDays() const;
        int InHours() const;
        int InMinutes() const;
        double InSecondsF() const;
        int64 InSeconds() const;
        double InMillisecondsF() const;
        int64 InMilliseconds() const;
        int64 InMillisecondsRoundedUp() const;
        int64 InMicroseconds() const;

        TimeDelta& operator=(TimeDelta other)
        {
            delta_ = other.delta_;
            return *this;
        }

        // 与其它TimeDelta运算.
        TimeDelta operator+(TimeDelta other) const
        {
            return TimeDelta(delta_ + other.delta_);
        }
        TimeDelta operator-(TimeDelta other) const
        {
            return TimeDelta(delta_ - other.delta_);
        }

        TimeDelta& operator+=(TimeDelta other)
        {
            delta_ += other.delta_;
            return *this;
        }
        TimeDelta& operator-=(TimeDelta other)
        {
            delta_ -= other.delta_;
            return *this;
        }
        TimeDelta operator-() const
        {
            return TimeDelta(-delta_);
        }

        // 与整数运算, 只能做乘除法, 加减运算应该使用上面的操作.
        TimeDelta operator*(int64 a) const
        {
            return TimeDelta(delta_ * a);
        }
        TimeDelta operator/(int64 a) const
        {
            return TimeDelta(delta_ / a);
        }
        TimeDelta& operator*=(int64 a)
        {
            delta_ *= a;
            return *this;
        }
        TimeDelta& operator/=(int64 a)
        {
            delta_ /= a;
            return *this;
        }
        int64 operator/(TimeDelta a) const
        {
            return delta_ / a.delta_;
        }

        // 实现在后面, 因为依赖其它类的定义.
        Time operator+(Time t) const;
        TimeTicks operator+(TimeTicks t) const;

        // 比较操作.
        bool operator==(TimeDelta other) const
        {
            return delta_ == other.delta_;
        }
        bool operator!=(TimeDelta other) const
        {
            return delta_ != other.delta_;
        }
        bool operator<(TimeDelta other) const
        {
            return delta_ < other.delta_;
        }
        bool operator<=(TimeDelta other) const
        {
            return delta_ <= other.delta_;
        }
        bool operator>(TimeDelta other) const
        {
            return delta_ > other.delta_;
        }
        bool operator>=(TimeDelta other) const
        {
            return delta_ >= other.delta_;
        }

    private:
        friend class Time;
        friend class TimeTicks;
        friend TimeDelta operator*(int64 a, TimeDelta td);

        // 用微秒时间差构造TimeDelta对象. 函数私有化以避免用户直接构造.
        // 使用FromSeconds、FromMilliseconds等函数替换.
        explicit TimeDelta(int64 delta_us) : delta_(delta_us) {}

        // 微秒时间差.
        int64 delta_;
    };

    class Time
    {
    public:
        static const int64 kMillisecondsPerSecond = 1000;
        static const int64 kMicrosecondsPerMillisecond = 1000;
        static const int64 kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
            kMillisecondsPerSecond;
        static const int64 kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
        static const int64 kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
        static const int64 kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
        static const int64 kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
        static const int64 kNanosecondsPerMicrosecond = 1000;
        static const int64 kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
            kMicrosecondsPerSecond;

        // 拆分过的时间, 以便更好的格式化. 类似于Win32的SYSTEMTIME结构体.
        struct Exploded
        {
            int year;          // 四位数字表示年 "2007"
            int month;         // 从1起始的月份 (数值1==一月, 类推)
            int day_of_week;   // 从0起始的工作日 (0==星期日, 类推)
            int day_of_month;  // 从1起始的天 (1-31)
            int hour;          // 时 (0-23)
            int minute;        // 分 (0-59)
            int second;        // 秒 (0-59, 正闰秒可能会导致超过60)
            int millisecond;   // 毫秒 (0-999)

            // 简单的校验数据成员是否在各自范围以内. 'true'并不代表可以成功
            // 转换成一个Time.
            bool HasValidValues() const;
        };

        // NULL时间. 使用Time::Now()获取当前时间.
        explicit Time() : us_(0) {}

        // 如果对象没有初始化, 返回true.
        bool is_null() const
        {
            return us_ == 0;
        }

        // 返回类Unix操作系统的起始时间(Jan 1, 1970).
        static Time UnixEpoch();

        // 返回当前时间. 注意系统时间的改变会导致时间退后, 无法保证时间是严格递增
        // 的, 也就是无法保证两次调用Now()的时间一定是不同的.
        static Time Now();

        // 返回当前时间. 与Now()类似, 但这里始终使用系统时间, 所以返回的时间和系统
        // 时间是一致的.
        static Time NowFromSystemTime();

        // UTC的time_t和Time类之间的转换.
        static Time FromTimeT(time_t tt);
        time_t ToTimeT() const;

        // Time和double表示的自起始时间(Jan 1, 1970)计秒之间的转换. Webkit使用这种
        // 表示方式. 因为WebKit初始化double时间值0表示未初始化, 这里也映射一个空的
        // 对象表示未初始化.
        static Time FromDoubleT(double dt);
        double ToDoubleT() const;

        static Time FromFileTime(FILETIME ft);
        FILETIME ToFileTime() const;

        // 最小时间精度. windows平台上约等于15.6ms. 一些老的操作系统版本可能不一样,
        // 但这里一致对待.
        static const int kMinLowResolutionThresholdMs = 16;

        // 启用或者停用Windows的高精度时间. 停用高精度时间, 调用
        // ActivateHighResolutionTimer会失败. 当停用高精度时间时, 函数不会使之无效,
        // 而是后面的激活.
        // 必须在主线程中调用.
        static void EnableHighResolutionTimer(bool enable);

        // 根据|activate|标记激活高精度时间或者使之无效. 如果HighResolutionTimer
        // 停用(参见EnableHighResolutionTimer), 函数返回false. 否则返回true.
        // 所有激活高精度时间的调用必须配对调用使之无效.
        static bool ActivateHighResolutionTimer(bool activate);

        // 本地时间或者UTC转换到Time.
        static Time FromUTCExploded(const Exploded& exploded)
        {
            return FromExploded(false, exploded);
        }
        static Time FromLocalExploded(const Exploded& exploded)
        {
            return FromExploded(true, exploded);
        }

        // 转换整数值到Time, 用已知的兼容数据反序列化|Time|结构. 不提供这样的
        // 构造函数是因为从调用者的视角整数类型无法确定是合法的时间.
        static Time FromInternalValue(int64 us)
        {
            return Time(us);
        }

        // 从字符串转换到时间对象. 例如: "Tue, 15 Nov 1994 12:45:26 GMT".
        // 如果没有指定时区, 使用本地时间.
        static bool FromString(const wchar_t* time_string, Time* parsed_time);

        // 序列化, 用FromInternalValue可重新构造对象.
        // 不要直接做算术运算, 应该使用提供的操作函数.
        int64 ToInternalValue() const
        {
            return us_;
        }

        // 使用本地时间或者UTC填充拆分结构.
        void UTCExplode(Exploded* exploded) const
        {
            return Explode(false, exploded);
        }
        void LocalExplode(Exploded* exploded) const
        {
            return Explode(true, exploded);
        }

        // 时间向下取整到当天的午夜(半夜12点钟).
        Time LocalMidnight() const;

        Time& operator=(Time other)
        {
            us_ = other.us_;
            return *this;
        }

        // 计算时间差.
        TimeDelta operator-(Time other) const
        {
            return TimeDelta(us_ - other.us_);
        }

        // 修改时间.
        Time& operator+=(TimeDelta delta)
        {
            us_ += delta.delta_;
            return *this;
        }
        Time& operator-=(TimeDelta delta)
        {
            us_ -= delta.delta_;
            return *this;
        }

        // 返回新时间.
        Time operator+(TimeDelta delta) const
        {
            return Time(us_ + delta.delta_);
        }
        Time operator-(TimeDelta delta) const
        {
            return Time(us_ - delta.delta_);
        }

        // 比较操作.
        bool operator==(Time other) const
        {
            return us_ == other.us_;
        }
        bool operator!=(Time other) const
        {
            return us_ != other.us_;
        }
        bool operator<(Time other) const
        {
            return us_ < other.us_;
        }
        bool operator<=(Time other) const
        {
            return us_ <= other.us_;
        }
        bool operator>(Time other) const
        {
            return us_ > other.us_;
        }
        bool operator>=(Time other) const
        {
            return us_ >= other.us_;
        }

    private:
        friend class TimeDelta;

        // 拆分Time到本地时间(|is_local = true|)或者UTC(|is_local = false|).
        void Explode(bool is_local, Exploded* exploded) const;

        // 本地时间(|is_local = true|)或者UTC(|is_local = false|)组合成Time.
        static Time FromExploded(bool is_local, const Exploded& exploded);

        explicit Time(int64 us) : us_(us) {}

        // 表示起始时间Jan 1, 1970 UTC的微秒数, 具体平台相关.
        static const int64 kTimeTToMicrosecondsOffset;

        // 表示当前是否使用快速时间. 比如在电池供电时, 应该选择不使用高精度时间
        // 以节省用电.
        static bool high_resolution_timer_enabled_;

        // UTC微秒数表示时间.
        int64 us_;
    };

    inline Time TimeDelta::operator+(Time t) const
    {
        return Time(t.us_ + delta_);
    }

    // static
    inline TimeDelta TimeDelta::FromDays(int64 days)
    {
        return TimeDelta(days * Time::kMicrosecondsPerDay);
    }

    // static
    inline TimeDelta TimeDelta::FromHours(int64 hours)
    {
        return TimeDelta(hours * Time::kMicrosecondsPerHour);
    }

    // static
    inline TimeDelta TimeDelta::FromMinutes(int64 minutes)
    {
        return TimeDelta(minutes * Time::kMicrosecondsPerMinute);
    }

    // static
    inline TimeDelta TimeDelta::FromSeconds(int64 secs)
    {
        return TimeDelta(secs * Time::kMicrosecondsPerSecond);
    }

    // static
    inline TimeDelta TimeDelta::FromMilliseconds(int64 ms)
    {
        return TimeDelta(ms * Time::kMicrosecondsPerMillisecond);
    }

    // static
    inline TimeDelta TimeDelta::FromMicroseconds(int64 us)
    {
        return TimeDelta(us);
    }

    class TimeTicks
    {
    public:
        TimeTicks() : ticks_(0) {}

        // 平台相关的计数器表示"right now".
        // 时钟的分辨率是1-15ms, 具体依赖硬件/软件配置.
        static TimeTicks Now();

        // 返回平台相关的高分辨率计数器. 实现依赖硬件, 有可能返回比毫秒高的精度,
        // 但不一定. 这个函数调用比Now()耗时, 如无必要尽量不要使用.
        static TimeTicks HighResNow();

        // QPC计时的绝对时间差.
        static int64 GetQPCDriftMicroseconds();

        // 系统运行高精度时钟则返回true.
        static bool IsHighResClockWorking();

        // 如果对象没初始化, 返回true.
        bool is_null() const
        {
            return ticks_ == 0;
        }

        // 返回TimeTicks对象内部数值.
        int64 ToInternalValue() const
        {
            return ticks_;
        }

        TimeTicks& operator=(TimeTicks other)
        {
            ticks_ = other.ticks_;
            return *this;
        }

        // 计算差值.
        TimeDelta operator-(TimeTicks other) const
        {
            return TimeDelta(ticks_ - other.ticks_);
        }

        // 修改.
        TimeTicks& operator+=(TimeDelta delta)
        {
            ticks_ += delta.delta_;
            return *this;
        }
        TimeTicks& operator-=(TimeDelta delta)
        {
            ticks_ -= delta.delta_;
            return *this;
        }

        // 返回新值.
        TimeTicks operator+(TimeDelta delta) const
        {
            return TimeTicks(ticks_ + delta.delta_);
        }
        TimeTicks operator-(TimeDelta delta) const
        {
            return TimeTicks(ticks_ - delta.delta_);
        }

        // 比较操作.
        bool operator==(TimeTicks other) const
        {
            return ticks_ == other.ticks_;
        }
        bool operator!=(TimeTicks other) const
        {
            return ticks_ != other.ticks_;
        }
        bool operator<(TimeTicks other) const
        {
            return ticks_ < other.ticks_;
        }
        bool operator<=(TimeTicks other) const
        {
            return ticks_ <= other.ticks_;
        }
        bool operator>(TimeTicks other) const
        {
            return ticks_ > other.ticks_;
        }
        bool operator>=(TimeTicks other) const
        {
            return ticks_ >= other.ticks_;
        }

    protected:
        friend class TimeDelta;

        // 请使用Now()构造新对象. 本构造函数内部使用. 用微秒计数.
        explicit TimeTicks(int64 ticks) : ticks_(ticks) {}

        // 微秒计数器.
        int64 ticks_;

        typedef DWORD (*TickFunctionType)(void);
        static TickFunctionType SetMockTickFunction(TickFunctionType ticker);
    };

    inline TimeTicks TimeDelta::operator+(TimeTicks t) const
    {
        return TimeTicks(t.ticks_ + delta_);
    }

} //namespace base

#endif //__base_time_h__