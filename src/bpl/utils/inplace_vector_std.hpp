#pragma once

#include <array>
#include <exception>

// std::inplace_vector will be part of C++26
// Using a custom basic implementation until we can use the official class
namespace std {

template <typename T, size_t CAPACITY>
class inplace_vector {
   public:
	using size_type = size_t;

	inplace_vector() : size_(0) {}

	static constexpr size_type capacity() noexcept { return CAPACITY; }

	void push_back(const T& item) {
		if (size_ >= CAPACITY) {
			throw std::bad_alloc();
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
		if (count > CAPACITY) {
			throw std::bad_alloc();
		}
		if (count > size_) {
			for (size_type i = size_; i < count; ++i) {
				data_[i] = value;
			}
		}
		size_ = count;
	}

	const T* data() const { return data_.data(); }
	T* data() { return data_.data(); }

	auto begin() const { return data_.begin(); }
	auto end() const { return data_.begin() + size_; }

//   private:
	std::array<T, CAPACITY> data_;
	size_type size_;
};

};	// namespace std
