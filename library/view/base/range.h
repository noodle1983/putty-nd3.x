
#ifndef __view_range_h__
#define __view_range_h__

#pragma once

#include <windows.h>
#include <richedit.h>

#include <iosfwd>

namespace view
{

    // A Range contains two integer values that represent a numeric range, like the
    // range of characters in a text selection. A range is made of a start and end
    // position; when they are the same, the Range is akin to a caret. Note that
    // |start_| can be greater than |end_| to respect the directionality of the
    // range.
    class Range
    {
    public:
        // Creates an empty range {0,0}.
        Range();

        // Initializes the range with a start and end.
        Range(size_t start, size_t end);

        // Initializes the range with the same start and end positions.
        explicit Range(size_t position);

        // Platform constructors.
        // The |total_length| paramater should be used if the CHARRANGE is set to
        // {0,-1} to indicate the whole range.
        Range(const CHARRANGE& range, LONG total_length=-1);

        // Returns a range that is invalid, which is {size_t_max,size_t_max}.
        static const Range InvalidRange();

        // Checks if the range is valid through comparision to InvalidRange().
        bool IsValid() const;

        // Getters and setters.
        size_t start() const { return start_; }
        void set_start(size_t start) { start_ = start; }

        size_t end() const { return end_; }
        void set_end(size_t end) { end_ = end; }

        // Returns the absolute value of the length.
        size_t length() const
        {
            int length = end() - start();
            return length>=0 ? length : -length;
        }

        bool is_reversed() const { return start() > end(); }
        bool is_empty() const { return start() == end(); }

        // Returns the minimum and maximum values.
        size_t GetMin() const;
        size_t GetMax() const;

        bool operator==(const Range& other) const;
        bool operator!=(const Range& other) const;
        bool EqualsIgnoringDirection(const Range& other) const;

        CHARRANGE ToCHARRANGE() const;

    private:
        size_t start_;
        size_t end_;
    };

    std::ostream& operator<<(std::ostream& out, const Range& range);

} //namespace view

#endif //__view_range_h__