#ifndef PM_MEMORY_H
#define PM_MEMORY_H
#pragma once

#include <atomic>
#include <cstddef>
#include <iterator>
#include <type_traits>

/*
 * The purpose of this class is to provide an
 * owning pointer to a range of contiguous
 * memory. Similar to std::unique_ptr<T[]>,
 * but with some additional convenience.
 */

namespace pm
{
    template<typename T>
    struct span;

    template<typename T>
    struct memory
    {
    public:
        using value_t   = std::remove_cv_t<std::remove_reference_t<T>>;
        using index_t   = std::ptrdiff_t;
        using counter_t = std::atomic_int;

        struct iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = value_t;
            using difference_type   = index_t;
            using pointer           = value_t*;
            using reference         = value_t&;

            explicit iterator(value_t* ptr) noexcept
                : ptr{ ptr }
            {}

            iterator& operator ++() noexcept
            {
                this->ptr++;

                return *this;
            }

            reference operator *() const noexcept
            {
                return *(this->ptr);
            }

            bool operator == (iterator const &other) const noexcept
            {
                return (this->ptr == other.ptr);
            }

            bool operator != (iterator const &other) const noexcept
            {
                return (this->ptr != other.ptr);
            }

        private:
            pointer ptr;
        };

        static_assert(!(std::is_array_v<value_t> || std::is_pointer_v<value_t>), "Pointers and arrays are not supported!");

        memory(index_t size)
            : ptr        { new value_t[size] },
              length     { size              },
              ref_counter{ new counter_t{}   }
        {
            //We have one reference
            this->ref_counter->store(1);
        }

        memory(value_t* && data, index_t size) noexcept
            : ptr        { data            },
              length     { size            },
              ref_counter{ new counter_t{} }
        {
            //We have one reference
            this->ref_counter->store(1);
        }

        memory(memory<T> const &other) noexcept 
            : ptr        { other.ptr         },
              length     { other.length      },
              ref_counter{ other.ref_counter }
        {
            //Increment reference count
            this->ref_counter->fetch_add(1);
        }

        memory(memory<T> && other) noexcept
            : ptr        { other.ptr         },
              length     { other.length      },
              ref_counter{ other.ref_counter }
        {}

        ~memory()
        {
            //Decrement reference count
            if (this->ref_counter->fetch_sub(1) == 1)
            {
                //Delete array if last reference is dead
                delete[] ptr;
            }
        }

        auto begin() const noexcept
        {
            return iterator{ this->ptr };
        }

        auto end() const noexcept
        {
            return iterator{ this->ptr + this->length };
        }

        auto data() const noexcept
        {
            return this->ptr;
        }

        auto size() const noexcept
        {
            return this->length;
        }

        value_t& operator [](index_t idx) const noexcept
        {
            return *(this->ptr + idx);
        }

        memory<T>& operator ^= (span<T> const &other) noexcept
        {
            //Find the number of elements to XOR
            auto const count = std::min<index_t>(this->length, other.size());

            //Iterate over the values and perform the XOR
            for (index_t i = 0; i < count; i++) (*this)[i] ^= other[i];

            //Return ourselves
            return *this;
        }

        operator span<T>() const& noexcept
        {
            return span<T>{ this->ptr, this->length };
        }

        operator span<T>() && noexcept = delete;

    private:
        value_t*   const ptr;
        index_t    const length;
        counter_t* const ref_counter;
    };
};

#endif
