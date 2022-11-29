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

    struct LinearAllocatorData
    {
        size_t used;
        void *previousAllocation;
        size_t previousAllocationSize;

        constexpr LinearAllocatorData() noexcept
            : used(0),
              previousAllocation(nullptr), previousAllocationSize(0) {}

        virtual ~LinearAllocatorData() {}

        virtual uint8_t *getBuffer() = 0;
        virtual size_t getBufferSize() const = 0;

        virtual void clear(bool hard) noexcept = 0;
    };

    template <size_t S>
    struct FixedSizeLinearAllocatorData : public LinearAllocatorData
    {
        std::array<uint8_t, S> buffer;

        constexpr FixedSizeLinearAllocatorData() noexcept
            : LinearAllocatorData(), buffer() {}

        ~FixedSizeLinearAllocatorData() {}

        uint8_t *getBuffer() override
        {
            return buffer.data();
        }

        size_t getBufferSize() const override { return S; }

        void clear(bool hard) noexcept override
        {
            used = 0;
            previousAllocation = nullptr;
            previousAllocationSize = 0;
            if (hard)
            {
                buffer.fill(0);
            }
        }
    };

    template <typename T>
    class LinearAllocator
    {
    public:
        typedef T value_type;

        template <class T2>
        friend class LinearAllocator;

    private:
        LinearAllocatorData &data;

    public:
        LinearAllocator(LinearAllocatorData &a) noexcept : data(a) {}

        LinearAllocator() = delete;
        LinearAllocator(LinearAllocator &u) noexcept : data(u.data) {}

        ~LinearAllocator() {}

        template <class U>
        LinearAllocator(const LinearAllocator<U> &u) noexcept : data(u.data) {}

        template <class U>
        bool operator==(const LinearAllocator<U> &) const noexcept
        {
            return true;
        }
        template <class U>
        bool operator!=(const LinearAllocator<U> &) const noexcept
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