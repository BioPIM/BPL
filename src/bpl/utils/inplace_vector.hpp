#pragma once

namespace bpl {

namespace impl {

template <typename T, unsigned int N>
class inplace_vector {
   private:
	static constexpr unsigned int CAPACITY = (N + 7 & ~7);	// Round to next multiple of 8
	static constexpr bool CAPACITY_CHECK = false;

   public:
	using size_type = uint32_t;

	inplace_vector() : size_(0) {}

	static constexpr size_type capacity() noexcept { return N; }

	void push_back(const T& item) {
		if constexpr (CAPACITY_CHECK) {
			if (size_ >= CAPACITY) {
				return;	 // Should throw bad alloc exception, but not possible so just return instead
			}
		}
		data_[size_] = item;
		size_++;
	}

	T& operator[](size_type idx) { return data_[idx]; }
	const T& operator[](size_type idx) const { return data_[idx]; }

	size_type size() const { return size_; }

	void reserve(size_type new_capacity) {}	 // Does nothing, only for compatibility with vector

	void resize(size_type count) { resize(count, T{}); }
	void resize(size_type count, const T& value) {
		if constexpr (CAPACITY_CHECK) {
			if (count > CAPACITY) {
				return;	 // Should throw bad alloc exception, but not possible so just return instead
			}
		}
		if (count > size_) {
			for (size_type i = size_; i < count; ++i) {
				data_[i] = value;
			}
		}
		size_ = count;
	}

	const T* data() const { return data_; }
	T* data() { return data_; }

	struct iterator {
		const inplace_vector& ref_;
		ARCH_ALIGN size_type idx_ = 0;

		iterator(const inplace_vector& ref, size_type offset) : ref_(ref), idx_(offset) {}

		iterator operator+(int32_t offset) {
			iterator other(*this);
			other.idx_ += offset;
			return other;
		}

		bool operator!=(const iterator& other) const { return idx_ != other.idx_; }

		const T& operator*() const { return ref_[idx_]; }

		void operator++() { ++idx_; }
	};

	iterator begin() const { return iterator(*this, 0); }
	iterator end() const { return iterator(*this, size()); }

	static constexpr bool parseable = false;

//   private:
	ARCH_ALIGN T data_[CAPACITY];
	size_type size_;

	static_assert(std::is_trivial_v<T>, "This inplace_vector implementation only supports trivial types");
};

};	// namespace impl
};	// namespace bpl

