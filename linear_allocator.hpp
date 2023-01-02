#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>

namespace TemAllocator
{
    constexpr bool isPowerOfTwo(size_t x)
    {
        return (x & (x - 1)) == 0;
    }

    constexpr size_t alignForward(size_t ptr, const size_t align)
    {
        const size_t modulo = ptr & (align - 1);
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
        size_t previousAllocationSize;

        constexpr LinearAllocatorData() noexcept
            : used(0), previousAllocationSize(0) {}

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

        constexpr size_t getBufferSize() const { return buffer.size(); }

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

        static size_t calculatePadding(size_t current)
        {
            const auto multiplier = (current / alignof(T)) + 1;
            const auto alignedAddress = multiplier * alignof(T);
            const auto padding = alignedAddress - current;
            return padding;
        }

        class bad_alloc : public std::exception
        {
        public:
            bad_alloc() throw() {}

#if __cplusplus >= 201103L
            bad_alloc(const bad_alloc &) = default;
            bad_alloc &operator=(const bad_alloc &) = default;
#endif

            virtual ~bad_alloc() throw()
            {
            }

            virtual const char *what() const throw()
            {
                return "Failed to allocate from TemLang linear allocator";
            }
        };

        T *allocate(size_t count = 1)
        {
            if (count == 0)
            {
                return nullptr;
            }

            const size_t size = sizeof(T) * count;
            if (size > data.getBufferSize())
            {
                throw bad_alloc();
            }

            uint8_t *buffer = data.getBuffer();
            const size_t currentAddress =
                reinterpret_cast<size_t>(buffer) + static_cast<size_t>(data.used);

            size_t padding = 0;
            if ((data.used % alignof(T)) != 0)
            {
                padding = calculatePadding(currentAddress);
            }

            if (data.used + padding + size > data.getBufferSize())
            {
                clear();
                return allocate(count);
            }

            data.used += padding;
            const size_t nextAddress = currentAddress + padding;
            data.used += size;
            data.previousAllocationSize = size;
            return reinterpret_cast<T *>(nextAddress);
        }

        T *reallocate(T *oldPtr, size_t count = 1)
        {
            const size_t newSize = sizeof(T) * count;
            if (newSize > data.getBufferSize())
            {
                throw bad_alloc();
            }

            uint8_t *buffer = data.getBuffer();
            void *previousAllocation = &buffer[data.used - data.previousAllocationSize];
            if (oldPtr != nullptr && previousAllocation == oldPtr)
            {
                if (data.previousAllocationSize > newSize)
                {
                    data.used -= data.previousAllocationSize - newSize;
                }
                else
                {
                    const size_t diff = newSize - data.previousAllocationSize;
                    if (data.used + diff <= data.getBufferSize())
                    {
                        goto doAllocation;
                    }
                    data.used += diff;
                }
                data.previousAllocationSize = newSize;
                return oldPtr;
            }

        doAllocation:
            T *newData = allocate(count);

            if (newData != nullptr && oldPtr != nullptr)
            {
                memmove(newData, oldPtr, newSize);
            }
            return newData;
        }

        void deallocate(void *, const size_t) noexcept {}
    };
}