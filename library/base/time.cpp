
#include "time.h"

#include "third_party/nspr/prtime.h"

#include "cpu.h"
#include "memory/singleton.h"
#include "synchronization/lock.h"
#include "sys_string_conversions.h"
#include "threading/platform_thread.h"

#pragma comment(lib, "winmm.lib")

namespace base
{

    int TimeDelta::InDays() const
    {
        return static_cast<int>(delta_ / Time::kMicrosecondsPerDay);
    }

    int TimeDelta::InHours() const
    {
        return static_cast<int>(delta_ / Time::kMicrosecondsPerHour);
    }

    int TimeDelta::InMinutes() const
    {
        return static_cast<int>(delta_ / Time::kMicrosecondsPerMinute);
    }

    double TimeDelta::InSecondsF() const
    {
        return static_cast<double>(delta_) / Time::kMicrosecondsPerSecond;
    }

    int64 TimeDelta::InSeconds() const
    {
        return delta_ / Time::kMicrosecondsPerSecond;
    }

    double TimeDelta::InMillisecondsF() const
    {
        return static_cast<double>(delta_) / Time::kMicrosecondsPerMillisecond;
    }

    int64 TimeDelta::InMilliseconds() const
    {
        return delta_ / Time::kMicrosecondsPerMillisecond;
    }

    int64 TimeDelta::InMillisecondsRoundedUp() const
    {
        return (delta_ + Time::kMicrosecondsPerMillisecond - 1) /
            Time::kMicrosecondsPerMillisecond;
    }

    int64 TimeDelta::InMicroseconds() const
    {
        return delta_;
    }

    // static
    Time Time::FromTimeT(time_t tt)
    {
        if(tt == 0)
        {
            return Time(); // 维持0表示不存在.
        }
        return Time((tt*kMicrosecondsPerSecond) + kTimeTToMicrosecondsOffset);
    }

    time_t Time::ToTimeT() const
    {
        if(us_ == 0)
        {
            return 0; // 维持0表示不存在.
        }
        return (us_-kTimeTToMicrosecondsOffset) / kMicrosecondsPerSecond;
    }

    // static
    Time Time::FromDoubleT(double dt)
    {
        if(dt == 0)
        {
            return Time(); // 维持0表示不存在.
        }
        return Time(static_cast<int64>((dt *
            static_cast<double>(kMicrosecondsPerSecond)) +
            kTimeTToMicrosecondsOffset));
    }

    double Time::ToDoubleT() const
    {
        if(us_ == 0)
        {
            return 0; // 维持0表示不存在.
        }
        return (static_cast<double>(us_-kTimeTToMicrosecondsOffset) /
            static_cast<double>(kMicrosecondsPerSecond));
    }

    // static
    Time Time::UnixEpoch()
    {
        Time time;
        time.us_ = kTimeTToMicrosecondsOffset;
        return time;
    }

    Time Time::LocalMidnight() const
    {
        Exploded exploded;
        LocalExplode(&exploded);
        exploded.hour = 0;
        exploded.minute = 0;
        exploded.second = 0;
        exploded.millisecond = 0;
        return FromLocalExploded(exploded);
    }

    // static
    bool Time::FromString(const wchar_t* time_string, Time* parsed_time)
    {
        DCHECK((time_string!=NULL) && (parsed_time!=NULL));
        std::string ascii_time_string = SysWideToUTF8(time_string);
        if(ascii_time_string.length() == 0)
        {
            return false;
        }
        PRTime result_time = 0;
        PRStatus result = PR_ParseTimeString(ascii_time_string.c_str(),
            PR_FALSE, &result_time);
        if(PR_SUCCESS != result)
        {
            return false;
        }
        result_time += kTimeTToMicrosecondsOffset;
        *parsed_time = Time(result_time);
        return true;
    }

    inline bool is_in_range(int value, int lo, int hi)
    {
        return lo<=value && value<=hi;
    }

    bool Time::Exploded::HasValidValues() const
    {
        return is_in_range(month, 1, 12) &&
            is_in_range(day_of_week, 0, 6) &&
            is_in_range(day_of_month, 1, 31) &&
            is_in_range(hour, 0, 23) &&
            is_in_range(minute, 0, 59) &&
            is_in_range(second, 0, 60) &&
            is_in_range(millisecond, 0, 999);
    }

} //namespace base


// Windows计时器基本介绍
//
// 一篇不错的文章: http://www.ddj.com/windows/184416651
// 一个mozilla bug: http://bugzilla.mozilla.org/show_bug.cgi?id=363258
//
// Windows缺省的计数器, GetSystemTimeAsFileTime精度并不高.
// 最好情况大约是15.5ms.
//
// QueryPerformanceCounter是实现高精度时钟不错的选择, 但在某些硬件上有问题,
// 有的时候会跳跃. 笔记本上, QPC调用代价很高. 台式机上比timeGetTime()慢3-4
// 倍, 笔记本上会慢10倍.
//
// 另外一种选择就是使用timeGetTime(), 精度是1ms, 但是只要调用APIs(
// timeBeginPeriod())就会影响系统上其它的应用. 缺省精度只有15.5ms.
// 因此很不幸, 我们不能调用timeBeginPeriod因为不想影响其它应用.
// 再者在移动平台调用更快的多媒体计时器会损害电池寿命. 参见intel文章:
// http://softwarecommunity.intel.com/articles/eng/1086.htm
//
// 为了解决问题, 还将使用timeGetTime(), 只在不是以电池供电运行的时候才提高
// 系统的计数器精度. 使用timeBeginPeriod(1)以便消息循环等待和时间测量精度
// 保持一致, 否则WaitForSingleObject(..., 1)在不被唤醒的时候最少堵塞15ms.

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace
{

    // 据MSDN: FILETIME "用64位数值表示自January 1, 1601 (UTC)以来的百纳秒数".
    int64 FileTimeToMicroseconds(const FILETIME& ft)
    {
        // bit_cast解决字节对齐问题, 除以10转换百纳秒到毫秒.
        // 仅在适合little-endian机器.
        return bit_cast<int64, FILETIME>(ft) / 10;
    }

    void MicrosecondsToFileTime(int64 us, FILETIME* ft)
    {
        DCHECK(us >= 0) << "Time is less than 0, negative values are not "
            "representable in FILETIME";

        // 乘以10转换毫秒到百纳秒. bit_cast解决字节对齐问题.
        // 仅在适合little-endian机器.
        *ft = bit_cast<FILETIME, int64>(us * 10);
    }

    int64 CurrentWallclockMicroseconds()
    {
        FILETIME ft;
        ::GetSystemTimeAsFileTime(&ft);
        return FileTimeToMicroseconds(ft);
    }

    // 时钟频率重采样的时间间隔. 60秒.
    const int kMaxMillisecondsToAvoidDrift = 60 * Time::kMillisecondsPerSecond;

    int64 initial_time = 0;
    TimeTicks initial_ticks;

    void InitializeClock()
    {
        initial_ticks = TimeTicks::Now();
        initial_time = CurrentWallclockMicroseconds();
    }

} //namespace

// Time的内部用FILETIME表示, 起始于1601-01-01 00:00:00 UTC.
// ((1970-1601)*365+89)*24*60*60*1000*1000, 其中的89是1601到1970的闰年天数总和:
// (1970-1601)/4不包括1700, 1800, and 1900.
// static
const int64 Time::kTimeTToMicrosecondsOffset = GG_INT64_C(11644473600000000);

bool Time::high_resolution_timer_enabled_ = false;
int Time::high_resolution_timer_activated_ = 0;

// static
Time Time::Now()
{
    if(initial_time == 0)
    {
        InitializeClock();
    }

    // 高精度计数器实现计时, 能够得到比10-15ms小的超时. 仅通过
    // CurrentWallclockMicroseconds(), 无法得到细粒度的定时器.
    //
    // 使用时, 初始化时钟(initial_time)和计数器(initial_ctr). 通过
    // 和初始时钟比较能得到消逝的时钟数, 然后就能得出时间差.
    //
    // 为避免误差, 计数器定期的和系统时钟同步.
    while(true)
    {
        TimeTicks ticks = TimeTicks::Now();

        // 计算自开始以来的时间计数.
        TimeDelta elapsed = ticks - initial_ticks;

        // 检查是否需要同步时钟.
        if(elapsed.InMilliseconds() > kMaxMillisecondsToAvoidDrift)
        {
            InitializeClock();
            continue;
        }

        return Time(elapsed + Time(initial_time));
    }
}

// static
Time Time::NowFromSystemTime()
{
    // 强制同步.
    InitializeClock();
    return Time(initial_time);
}

// static
Time Time::FromFileTime(FILETIME ft)
{
    return Time(FileTimeToMicroseconds(ft));
}

FILETIME Time::ToFileTime() const
{
    FILETIME utc_ft;
    MicrosecondsToFileTime(us_, &utc_ft);
    return utc_ft;
}

// static
void Time::EnableHighResolutionTimer(bool enable)
{
    // 检查访问线程是否唯一.
    static base::PlatformThreadId my_thread = base::PlatformThread::CurrentId();
    DCHECK(base::PlatformThread::CurrentId() == my_thread);

    if(high_resolution_timer_enabled_ == enable)
    {
        return;
    }

    high_resolution_timer_enabled_ = enable;
}

// static
bool Time::ActivateHighResolutionTimer(bool activate)
{
    if(!high_resolution_timer_enabled_)
    {
        return false;
    }

    // 除1ms以外, 可以使用其它值设置计时粒度.
    const int kMinTimerIntervalMs = 1;
    MMRESULT result;
    if(activate)
    {
        result = timeBeginPeriod(kMinTimerIntervalMs);
        high_resolution_timer_activated_++;
    }
    else
    {
        result = timeEndPeriod(kMinTimerIntervalMs);
        high_resolution_timer_activated_--;
    }
    return result == TIMERR_NOERROR;
}

// static
bool Time::IsHighResolutionTimerInUse()
{
    // Note:  we should track the high_resolution_timer_activated_ value
    // under a lock if we want it to be accurate in a system with multiple
    // message loops.  We don't do that - because we don't want to take the
    // expense of a lock for this.  We *only* track this value so that unit
    // tests can see if the high resolution timer is on or off.
    return high_resolution_timer_enabled_ &&
        high_resolution_timer_activated_>0;
}

// static
Time Time::FromExploded(bool is_local, const Exploded& exploded)
{
    // 创建系统拆分时间结构, 代表本地时间或者UTC.
    SYSTEMTIME st;
    st.wYear = exploded.year;
    st.wMonth = exploded.month;
    st.wDayOfWeek = exploded.day_of_week;
    st.wDay = exploded.day_of_month;
    st.wHour = exploded.hour;
    st.wMinute = exploded.minute;
    st.wSecond = exploded.second;
    st.wMilliseconds = exploded.millisecond;

    // 转换到FILETIME.
    FILETIME ft;
    if(!SystemTimeToFileTime(&st, &ft))
    {
        NOTREACHED() << "Unable to convert time";
        return Time(0);
    }

    // 确保是UTC.
    if(is_local)
    {
        FILETIME utc_ft;
        LocalFileTimeToFileTime(&ft, &utc_ft);
        return Time(FileTimeToMicroseconds(utc_ft));
    }
    return Time(FileTimeToMicroseconds(ft));
}

void Time::Explode(bool is_local, Exploded* exploded) const
{
    // FILETIME是UTC.
    FILETIME utc_ft;
    MicrosecondsToFileTime(us_, &utc_ft);

    // FILETIME转换到本地时间.
    BOOL success = TRUE;
    FILETIME ft;
    if(is_local)
    {
        success = FileTimeToLocalFileTime(&utc_ft, &ft);
    }
    else
    {
        ft = utc_ft;
    }

    // FILETIME到SYSTEMTIME(拆分).
    SYSTEMTIME st;
    if(!success || !FileTimeToSystemTime(&ft, &st))
    {
        NOTREACHED() << "Unable to convert time, don't know why";
        ZeroMemory(exploded, sizeof(exploded));
        return;
    }

    exploded->year = st.wYear;
    exploded->month = st.wMonth;
    exploded->day_of_week = st.wDayOfWeek;
    exploded->day_of_month = st.wDay;
    exploded->hour = st.wHour;
    exploded->minute = st.wMinute;
    exploded->second = st.wSecond;
    exploded->millisecond = st.wMilliseconds;
}

namespace
{

    // We define a wrapper to adapt between the __stdcall and __cdecl call of the
    // mock function, and to avoid a static constructor.  Assigning an import to a
    // function pointer directly would require setup code to fetch from the IAT.
    DWORD timeGetTimeWrapper()
    {
        return timeGetTime();
    }

    DWORD (*tick_function)(void) = &timeGetTimeWrapper;

    // 由于延迟累计丢失的时间(单位是毫秒).
    int64 rollover_ms = 0;

    // 最后一次timeGetTime的值, 用于检测延迟.
    DWORD last_seen_now = 0;

    // rollover_ms和last_seen_now的保护锁.
    // 注意: 一般来说应该避免使用全局对象, 这里是底层代码, 不想使用Singletons.
    // (Singleton非常简单以至于不需要对它有任何了解, 这往往也会导致一些陷阱).
    // Singleton对启动时间的影响是微不足道的, 只取决于底层代码本质.
    base::Lock rollover_lock;

    // 通过timeGetTime()实现TimeTicks::Now(). 返回值是Windows自启动以来总毫秒数,
    // 32位平台上每大约49天重置, 这带来一定的问题. 我们试着自己重置, 检查每49天
    // TimeTicks::Now()是否被调用过.
    TimeDelta RolloverProtectedNow()
    {
        base::AutoLock locked(rollover_lock);
        // 调用tick_function时需要加锁确保last_seen_now保持同步.
        DWORD now = tick_function();
        if(now < last_seen_now)
        {
            rollover_ms += 0x100000000I64; // ~49.7 days.
        }
        last_seen_now = now;
        return TimeDelta::FromMilliseconds(now + rollover_ms);
    }

    // 时钟计数器概述:
    // (1) CPU时钟计数器. (通过RDTSC获取)
    // CPU的计数器提供了最高精度的时间戳且代价最低. 然而, CPU的计数器是不可靠的,
    // 不应该在产品中使用. 最大的问题是处理器之间是独立的, 没有同步. 而且有些电脑
    // 上, 计数器会根据温度和电量改变频率, 有些情况下会停止.
    //
    // (2) QueryPerformanceCounter (QPC). QPC计数器提供了高分辨率(百纳秒)的时间戳,
    // 但代价比较昂贵. QueryPerformanceCounter实际调用的是HAL(通过ACPI).
    // 按照http://blogs.msdn.com/oldnewthing/archive/2005/09/02/459952.aspx,
    // 最坏的情况下, 是通过软中断时钟模拟实现. 最好的情况下, HAL推断RDTSC计数器的
    // 频率为常量, 直接使用. 在多处理器机器上, 尝试验证每个处理器返回的RDTSC是否
    // 一致, 并处理一些已知的硬件问题. 换言之, QPC在多处理器机器上尝试给出一致的
    // 结果, 但由于一些BIOS或者HAL的bugs并不完全可靠, 尤其是一些老的电脑上. 最近
    // 更新的HAL和较新的BIOS上, QPC已经比较可靠但使用时仍需多加小心.
    //
    // (3) 系统时间. 系统时间提供了低分辨率(一般10-55ms)的时间戳, 代价较低且非常
    // 可靠.
    class HighResNowSingleton
    {
    public:
        static HighResNowSingleton* GetInstance()
        {
            return Singleton<HighResNowSingleton>::get();
        }

        bool IsUsingHighResClock()
        {
            return ticks_per_microsecond_ != 0.0;
        }

        void DisableHighResClock()
        {
            ticks_per_microsecond_ = 0.0;
        }

        TimeDelta Now()
        {
            if(IsUsingHighResClock())
            {
                return TimeDelta::FromMicroseconds(UnreliableNow());
            }

            // 回退到慢时钟.
            return RolloverProtectedNow();
        }

        int64 GetQPCDriftMicroseconds()
        {
            if(!IsUsingHighResClock())
            {
                return 0;
            }

            return abs((UnreliableNow() - ReliableNow()) - skew_);
        }

    private:
        HighResNowSingleton() : ticks_per_microsecond_(0.0), skew_(0)
        {
            InitializeClock();

            // 在Athlon X2 CPU(例如: model 15)上, QueryPerformanceCounter是
            // 不可靠的. 使用低分辨率的时钟.
            base::CPU cpu;
            if(cpu.vendor_name()=="AuthenticAMD" && cpu.family()==15)
            {
                DisableHighResClock();
            }
        }

        // QPC时钟和GetSystemTimeAsFileTime同步.
        void InitializeClock()
        {
            LARGE_INTEGER ticks_per_sec = { 0 };
            if(!QueryPerformanceFrequency(&ticks_per_sec))
            {
                return; // 返回, 不保证函数正常.
            }
            ticks_per_microsecond_ = static_cast<float>(ticks_per_sec.QuadPart) /
                static_cast<float>(Time::kMicrosecondsPerSecond);

            skew_ = UnreliableNow() - ReliableNow();
        }

        // 返回开机以来的微秒数, 不一定精确.
        int64 UnreliableNow()
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return static_cast<int64>(now.QuadPart / ticks_per_microsecond_);
        }

        // 返回开机以来的微秒数, 非常精确.
        int64 ReliableNow()
        {
            return RolloverProtectedNow().InMicroseconds();
        }

        // 缓存时钟频率->微秒. 假定时钟频率比1微秒快(1MHz, 才可以).
        float ticks_per_microsecond_;  // 0表示QPF失败, 无法使用.
        int64 skew_; // 低分辨率和高分辨率时钟直接的误差(用于调试).

        friend struct DefaultSingletonTraits<HighResNowSingleton>;
    };

}  //namespace

// static
TimeTicks::TickFunctionType TimeTicks::SetMockTickFunction(
    TickFunctionType ticker)
{
    TickFunctionType old = tick_function;
    tick_function = ticker;
    return old;
}

// static
TimeTicks TimeTicks::Now()
{
    return TimeTicks() + RolloverProtectedNow();
}

// static
TimeTicks TimeTicks::HighResNow()
{
    return TimeTicks() + HighResNowSingleton::GetInstance()->Now();
}

// static
int64 TimeTicks::GetQPCDriftMicroseconds()
{
    return HighResNowSingleton::GetInstance()->GetQPCDriftMicroseconds();
}

// static
bool TimeTicks::IsHighResClockWorking()
{
    return HighResNowSingleton::GetInstance()->IsUsingHighResClock();
}