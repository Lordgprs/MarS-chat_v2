#pragma once

#include "bad_range.h"
#include "bad_length.h"
#include <iostream>

template <typename T>
class dynamic_array final {
public:
	dynamic_array() = default;
	explicit dynamic_array(int length) : length_{ length } {
		if (length_ < 0) {
			throw bad_length{length};
		}

		if (length > 0) {
			data_ = new T[length];
		}
	}
	dynamic_array(const dynamic_array &other) {
		reallocate(other.get_length());
		for (int index = 0; index < length_; ++index) {
			data_[index] = other.data_[index];
		}
	}
	~dynamic_array() {
		delete[] data_;
	}
	dynamic_array &operator=(const dynamic_array &other) {
		if (this == &other) {
			return *this; // self-assigning check
		}

		reallocate(other.get_length());
		for (int index = 0; index < length_; ++index) {
			data_[index] = other.data_[index];
		}

		return *this;
	}
	T &operator[](int index) {
		if (index < 0 || index >= length_) {
			throw bad_range{index};
		}
		return data_[index];
	}
	const T &operator[](int index) const {
		if (index < 0 || index >= length_) {
			throw bad_range{index};
		}
		return data_[index];
	}
	void erase() {
		delete[] data_;
		length_ = 0;
		data_ = nullptr;
	}
	void reallocate(int new_length) {
		if (new_length <= 0) {
			throw bad_length{new_length};
		}

		erase();
		delete[] data_;
		data_ = new T[new_length];
		length_ = new_length;
	}
	void resize(int new_length) {
		if (new_length <= 0) {
			throw bad_length{new_length};
		}
		if (new_length == length_) {
			return;
		}
		if (new_length == 0) {
			erase();
			return;
		}

		T *temp = new T[new_length];
		int min = new_length > length_ ? length_ : new_length;
		for (int index = 0; index < min; ++index) {
			temp[index] = data_[index];
		}
		delete[] data_;
		data_ = temp;
		length_ = new_length;
	}
	void insertBefore(T value, int index) {
		if (index < 0 || index > length_) {
			throw bad_range{index};
		}

		T *temp = new T[length_ + 1];
		for (int i = 0; i < index; ++i) {
			temp[i] = data_[i];
		}
		temp[index] = value;
		for (int i = index + 1; i <= length_; ++i) {
			temp[i] = data_[i - 1];
		}
		delete[] data_;
		data_ = temp;
		++length_;
	}
	void pushFront(T value) {
		insertBefore(value, 0);
	}
	void pushBack(T value) {
		insertBefore(value, length_);
	}
	void remove(int index) {
		if (index < 0 || index >= length_) {
			throw bad_range{index};
		}
		if (length_ == 1) {
			erase();
			return;
		}

		T *temp = new T[length_ - 1];
		for (int i = 0; i < index; ++i) {
			temp[i] = data_[i];
		}
		for (int i = index + 1; i < length_; ++i) {
			temp[i - 1] = data_[i];
		}
		delete[] data_;
		data_ = temp;
		--length_;
	}
	int findFirst(T value) const {
		for (int i = 0; i < length_; ++i) {
			if (data_[i] == value) {
				return i;
			}
		}
		return -1;
	}
	int findLast(T value) const {
		for (int i = length_ - 1; i >= 0; --i) {
			if (data_[i] == value) {
				return i;
			}
		}
		return -1;
	}
	int getLength() const {
		return length_;
	}

private:
	int length_{ 0 };
	T *data_{ nullptr };
};
