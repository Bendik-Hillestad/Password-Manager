#ifndef PM_SPAN_H
#define PM_SPAN_H
#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace pm
{
    template<typename T>
    struct span
    {
    public:
        using value_t  = std::remove_cv_t<T>;
        using index_t  = std::ptrdiff_t;

        using iterator = struct const_iterator : public std::iterator
        <
            std::forward_iterator_tag,
            value_t, index_t, value_t const*, value_t const&
        >
        {
        public:
            explicit const_iterator(value_t const* ptr) noexcept
                : ptr{ ptr }
            {}

            const_iterator operator ++() noexcept
            {
                this->ptr++;

                return *this;
            }

            auto operator *() const noexcept
            {
                return *(this->ptr);
            }

            bool operator == (const_iterator const &other) const noexcept
            {
                return (this->ptr == other.ptr);
            }

            bool operator != (const_iterator const &other) const noexcept
            {
                return (this->ptr != other.ptr);
            }

        private:
            value_t const* ptr;
        };

        constexpr span() noexcept
            : ptr{ nullptr }, len{ 0 }
        {}

        constexpr span(value_t const* const &data, index_t length) noexcept
            : ptr{ data }, len{ length }
        {}

        constexpr span(value_t const* const &first, value_t const* const &last)
            : ptr{ first }, len{last - first}
        {}

        template<std::size_t N>
        constexpr span(value_t const(&arr)[N]) noexcept
            : ptr{ arr }, len{ N }
        {}

        constexpr auto begin() const noexcept
        {
            return iterator{ this->ptr };
        }

        constexpr auto end() const noexcept
        {
            return iterator{ this->ptr + this->len };
        }

        constexpr auto cbegin() const noexcept
        {
            return const_iterator{ this->ptr };
        }

        constexpr auto cend() const noexcept
        {
            return const_iterator{ this->ptr + this->len };
        }

        constexpr auto at(index_t offset) const noexcept
        {
            return *(this->ptr + offset);
        }

        constexpr auto slice(index_t offset) const noexcept
        {
            //Calculate the new length of the span
            auto const newlen = std::max<index_t>(0, len - offset);

            //Return the sliced span
            return span<value_t>(&this->ptr[offset], newlen);
        }

        constexpr auto slice(index_t offset, index_t count) const noexcept
        {
            //Calculate the new length of the span
            auto const newlen = std::max<index_t>(0, std::min<index_t>(len - offset, count));

            //Return the sliced span
            return span<value_t>{&this->ptr[offset], newlen};
        }

        constexpr void copy_to(value_t* const &dst, index_t size) const noexcept
        {
            //Calculate how many elements to copy
            auto const count = std::min<index_t>(this->len, size);

            //Copy the elements to the destination
            for (index_t i = 0; i < count; i++) dst[i] = this->ptr[i];
        }

        constexpr auto data() const noexcept
        {
            return this->ptr;
        }

        constexpr auto size() const noexcept
        {
            return this->len;
        }

        constexpr bool valid() const noexcept
        {
            return (this->ptr != nullptr) && (this->len > 0);
        }

    private:
        value_t const* ptr;
        index_t        len;
    };
};

#endif
