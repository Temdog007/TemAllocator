/******************************************************************************
	Copyright (C) 2022 by Temitope Alaga <temdog007@yaoo.com>
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "allocator.hpp"

#include <string>
#include <codecvt>
#include <locale>
#include <stack>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <set>
#include <list>
#include <cmath>
#include <deque>
#include <queue>

namespace TemAllocator
{
	using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
	using String32 = std::basic_string<char32_t, std::char_traits<char32_t>, Allocator<char32_t>>;

	using StringStream = std::basic_stringstream<char, std::char_traits<char>, Allocator<char>>;
	using OStringStream = std::basic_ostringstream<char, std::char_traits<char>, Allocator<char>>;
	using IStringStream = std::basic_istringstream<char, std::char_traits<char>, Allocator<char>>;

	template <typename T>
	using List = std::vector<T, Allocator<T>>;

	template <typename T>
	using Deque = std::deque<T, Allocator<T>>;

	template <typename K>
	using Set = std::unordered_set<K, std::hash<K>, std::equal_to<K>, Allocator<K>>;

	template <typename K>
	using OrderedSet = std::set<K, std::less<K>, Allocator<K>>;

	template <typename K, typename V>
	using Map = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, Allocator<std::pair<const K, V>>>;

	template <typename K, typename V>
	using OrderedMap = std::map<K, V, std::less<K>, Allocator<std::pair<const K, V>>>;

	template <typename T>
	using LinkedList = std::list<T, Allocator<T>>;

	template <typename T>
	using Queue = std::queue<T, LinkedList<T>>;

	template <typename T>
	using Stack = std::stack<T, Allocator<T>>;

	using UTF8Converter = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t, Allocator<char32_t>, Allocator<char>>;

	template <typename T, typename... Args>
	static inline T *allocateAndConstruct(Args &&...args)
	{
		Allocator<T> a;
		T *t = a.allocate(1);
		a.construct(t, std::forward<Args>(args)...);
		return t;
	}

	template <typename T>
	static inline void destroyAndDeallocate(T *const t)
	{
		Allocator<T> a;
		a.destroy(t);
		a.deallocate(t);
	}

	template <typename T>
	struct Deleter
	{
		constexpr Deleter() noexcept = default;
#if __unix__
		template <typename U, typename = std::_Require<std::is_convertible<U *, T *>>>
		constexpr Deleter(Deleter<U>) noexcept
		{
		}
#else
		template <typename U>
		constexpr Deleter(Deleter<U>) noexcept
		{
		}
#endif
		void operator()(T *t) const
		{
			destroyAndDeallocate(t);
		}
	};

	template <typename T>
	using unique_ptr = std::unique_ptr<T, Deleter<T>>;

	template <typename T>
	using shared_ptr = std::shared_ptr<T>;

	template <typename T>
	using weak_ptr = std::weak_ptr<T>;

	template <typename T, typename... Args>
	static inline unique_ptr<T> make_unique(Args &&...args)
	{
		return unique_ptr<T>(allocateAndConstruct<T>(std::forward<Args>(args)...), Deleter<T>());
	}

	template <typename T, typename... Args>
	static inline shared_ptr<T> make_shared(Args &&...args)
	{
		return shared_ptr<T>(allocateAndConstruct<T>(std::forward<Args>(args)...), Deleter<T>());
	}

	template <typename T>
	static inline shared_ptr<T> make_shared(T *t)
	{
		return shared_ptr<T>(t, Deleter<T>());
	}
} // namespace TemAllocator

namespace std
{
	template <>
	struct hash<TemAllocator::String>
	{
		std::size_t operator()(const TemAllocator::String &s) const noexcept
		{
			std::size_t value = 0;
			for (size_t i = 0; i < s.size(); ++i)
			{
				value += s[i] * std::pow(size_t(31), i);
			}
			return value;
		}
	};

	template <>
	struct hash<TemAllocator::String32>
	{
		std::size_t operator()(const TemAllocator::String32 &s) const noexcept
		{
			std::size_t value = 0;
			for (size_t i = 0; i < s.size(); ++i)
			{
				value += s[i] * std::pow(size_t(31), i);
			}
			return value;
		}
	};
} // namespace std
