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

    template <typename Data>
    struct LinearAllocatorData
    {
        size_t used;
        void *previousAllocation;
        size_t previousAllocationSize;

        constexpr LinearAllocatorData() noexcept
            : used(0),
              previousAllocation(nullptr), previousAllocationSize(0) {}

        uint8_t *getBuffer()
        {
            return static_cast<Data *>(this)->getBuffer();
        }

        size_t getBufferSize() const
        {
            return static_cast<const Data *>(this)->getBufferSize();
        }

        void clear(bool hard) noexcept
        {
            used = 0;
            previousAllocation = nullptr;
            previousAllocationSize = 0;
            static_cast<Data *>(this)->clear(hard);
        }
    };

    template <size_t S>
    struct FixedSizeLinearAllocatorData
        : public LinearAllocatorData<FixedSizeLinearAllocatorData<S>>
    {
        std::array<uint8_t, S> buffer;

        constexpr FixedSizeLinearAllocatorData() noexcept
            : LinearAllocatorData<FixedSizeLinearAllocatorData<S>>(),
              buffer() {}

        uint8_t *getBuffer()
        {
            return buffer.data();
        }

        size_t getBufferSize() const { return S; }

        void clear(bool hard) noexcept
        {
            if (hard)
            {
                buffer.fill(0);
            }
        }
    };

    template <typename T, typename D>
    class LinearAllocator
    {
    public:
        typedef T value_type;

        template <class T2, class D2>
        friend class LinearAllocator;

    private:
        LinearAllocatorData<D> &data;

    public:
        LinearAllocator(LinearAllocatorData<D> &a) noexcept : data(a) {}

        LinearAllocator() = delete;
        LinearAllocator(const LinearAllocator &u) noexcept : data(u.data) {}
        LinearAllocator(LinearAllocator &&u) noexcept : data(u.data) {}

        ~LinearAllocator() {}

        template <class U>
        LinearAllocator(const LinearAllocator<U, D> &u) noexcept : data(u.data) {}
        template <class U>
        LinearAllocator(LinearAllocator<U, D> &&u) noexcept : data(u.data) {}

        template <class U>
        bool operator==(const LinearAllocator<U, D> &) const noexcept
        {
            return true;
        }
        template <class U>
        bool operator!=(const LinearAllocator<U, D> &) const noexcept
        {
            return false;
        }

        /**
         * @brief Get total availble memory
         *
         * @return total available memory in bytes
         */
        size_t getTotal() const noexcept
        {
            return data.getBufferSize();
        }

        /**
         * @brief Get amount of memory currently in use
         *
         * @return memory in use in bytes
         */
        size_t getUsed() const noexcept
        {
            return data.used;
        }

        /**
         * @brief Clear all allocations
         */
        void clear(bool hard = false) noexcept
        {
            data.clear(hard);
        }

        T *allocate(size_t count = 1) noexcept
        {
            if (count == 0)
            {
                return nullptr;
            }

            const size_t size = sizeof(T) * count;
            uint8_t *buffer = data.getBuffer();
            const uintptr_t start = reinterpret_cast<uintptr_t>(buffer);
            const uintptr_t current = start + static_cast<uintptr_t>(data.used);
            uintptr_t used = alignForward(current, Alignment);
            used -= start;

            if (size > data.getBufferSize())
            {
                return nullptr;
            }
            if (used + size > data.getBufferSize())
            {
                clear();
            }

            T *ptr = reinterpret_cast<T *>(&buffer[used]);
            data.used = used + size;
            data.previousAllocation = ptr;
            data.previousAllocationSize = size;
            return ptr;
        }

        T *reallocate(T *oldPtr, size_t count = 1) noexcept
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
                    if (data.used + diff > data.getBufferSize())
                    {
                        return nullptr;
                    }
                    data.used += diff;
                    uint8_t *buffer = data.getBuffer();
                    memset(&buffer[data.used], 0, diff);
                }
                data.previousAllocationSize = newSize;
                return oldPtr;
            }

            T *newData = allocate(newSize);
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

        void deallocate(void *, const size_t) noexcept {}

        size_t saveState() noexcept
        {
            return data.used;
        }

        void restore(const size_t s) noexcept
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