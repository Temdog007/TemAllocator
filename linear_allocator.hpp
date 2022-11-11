#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>

namespace TemAllocator
{
    extern const size_t Alignment;

    constexpr bool isPowerOfTwo(uintptr_t x)
    {
        return (x & (x - 1)) == 0;
    }

    constexpr uintptr_t alignForward(uintptr_t ptr, const uintptr_t align)
    {
        const uintptr_t modulo = ptr & (align - 1);
        if (modulo != 0)
        {
            ptr += align - modulo;
        }
        return ptr;
    }

    template <typename T, size_t S>
    class LinearAllocator
    {
    public:
        struct Data
        {
            std::array<uint8_t, S> buffer;
            size_t used;
            void *previousAllocation;
            size_t previousAllocationSize;
        };

    private:
        Data &data;

    public:
        LinearAllocator(Data &data) noexcept : data(data) {}
        LinearAllocator(const LinearAllocator &) = delete;
        LinearAllocator(LinearAllocator &&) = delete;

        ~LinearAllocator() {}

        template <class U>
        LinearAllocator(const LinearAllocator<U, S> &u) noexcept : data(u.data) {}
        template <class U>
        bool operator==(const LinearAllocator<U, S> &) const noexcept
        {
            return true;
        }
        template <class U>
        bool operator!=(const LinearAllocator<U, S> &) const noexcept
        {
            return false;
        }

        /**
         * @brief Get total availble memory
         *
         * @return total available memory in bytes
         */
        size_t getTotal() const
        {
            return S;
        }

        /**
         * @brief Get amount of memory currently in use
         *
         * @return memory in use in bytes
         */
        size_t getUsed() const
        {
            return data.used;
        }

        /**
         * @brief Clear all allocations
         */
        void clear()
        {
            data.used = 0;
        }

        T *allocate(size_t count = 1)
        {
            if (count == 0)
            {
                return nullptr;
            }

            const size_t size = sizeof(T) * count;
            const uintptr_t current = reinterpret_cast<uintptr_t>(data.buffer.data()) + static_cast<uintptr_t>(data.used);
            uintptr_t used = alignForward(current, Alignment);
            used -= static_cast<uintptr_t>(data.buffer.data());

            if (used + size > S)
            {
                return nullptr;
            }

            T *ptr = reinterpret_cast<T *>(&data.buffer[used]);
            data.used = used + size;
            data.previousAllocation = ptr;
            data.previousAllocationSize = size;
            return ptr;
        }

        T *reallocate(T *oldPtr, size_t count = 1)
        {
            const size_t newSize = sizeof(T) * count;
            if (oldPtr != nullptr && data.previousAllocation == oldPtr)
            {
                if (data.previousAllocationSize > newSize)
                {
                    data.used -= data.previousAllocationSize - newSize;
                }
                else
                {
                    const size_t diff = newSize - data.previousAllocationSize;
                    if (data.used + diff > S)
                    {
                        return nullptr;
                    }
                    data.used += diff;
                    memset(&data.buffer[data.used], 0, diff);
                }
                data.previousAllocationSize = newSize;
                return oldPtr;
            }

            void *newData = allocate(newSize);
            if (newData == nullptr)
            {
                return nullptr;
            }

            if (oldPtr != nullptr)
            {
                memmove(newData, oldPtr, newSize);
            }
            return newData;
        }

        void deallocate(void *, const size_t) {}

        size_t saveState()
        {
            return data.used;
        }

        void restore(const size_t s)
        {
            if (s < data.used)
            {
                data.used = s;
                data.previousAllocation = nullptr;
                data.previousAllocationSize = 0;
            }
        }
    };
}