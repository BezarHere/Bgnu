#pragma once
#include "Blob.hpp"

class Buffer
{
public:
	typedef void *ptr_type;
	typedef const void *c_ptr_type;

	struct allocator
	{
		inline ptr_type operator()(const size_t size) const {
			return ::operator new[](size);
		}
	};

	struct deleter
	{
		inline void operator()(const ptr_type data) const {
			::operator delete[](data);
		}
	};

	typedef deleter allocator_type;
	typedef deleter deleter_type;

	inline Buffer() = default;
	inline ~Buffer() noexcept {
		deleter_type()(m_ptr);
	}

	// creates a buffer with size 
	inline explicit Buffer(const size_t size) : m_size{size}, m_ptr{allocator()(size)} {
	}

	// will copy the content from `ptr`
	inline Buffer(const c_ptr_type ptr, const size_t size) : Buffer(size) {
		memcpy(m_ptr, ptr, m_size);
	}

	// an empty buffer (equal to the default constructor)
	inline Buffer(std::nullptr_t) : m_size{0}, m_ptr{nullptr} {
	}

	inline Buffer(const Buffer &copy) : Buffer(copy.m_ptr, copy.m_size) {
	}

	inline Buffer(Buffer &&move) noexcept : m_size{move.m_size}, m_ptr{move.m_ptr} {
		move.m_ptr = nullptr;
	}

	inline Buffer &operator=(const Buffer &copy) {
		if (this == std::addressof(copy))
		{
			return *this;
		}
		deleter_type()(m_ptr);

		m_size = copy.m_size;
		m_ptr = allocator()(m_size);
		memcpy(m_ptr, copy.m_ptr, m_size);

		return *this;
	}

	inline Buffer &operator=(Buffer &&move) noexcept {
		deleter_type()(m_ptr);

		m_size = move.m_size;
		m_ptr = move.m_ptr;

		move.m_ptr = nullptr;
		return *this;
	}

	inline size_t size() const noexcept { return m_size; }
	// is the size equal to zero?
	inline bool empty() const noexcept { return m_size == 0; }
	// is the ptr valid? i.e not null
	inline bool valid() const noexcept { return m_ptr != nullptr; }
	// is the size greater then zero and the ptr not null?
	inline bool useable() const noexcept { return !empty() && valid(); }

	inline ptr_type get() noexcept { return m_ptr; }
	inline c_ptr_type get() const noexcept { return m_ptr; }

	inline operator ptr_type() noexcept { return m_ptr; }
	inline operator c_ptr_type() const noexcept { return m_ptr; }

	template <typename _T>
	inline Blob<_T> to_blob() { return {reinterpret_cast<_T *>(m_ptr), m_size / sizeof(_T)}; }

	template <typename _T>
	inline Blob<const _T> to_blob() const { return {reinterpret_cast<const _T *>(m_ptr), m_size / sizeof(_T)}; }

	// resets the buffer to be invalid and returned a new ptr to be deleter by caller
	inline ptr_type release() {
		const ptr_type ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}

private:
	size_t m_size = 0;
	ptr_type m_ptr = nullptr;
};
