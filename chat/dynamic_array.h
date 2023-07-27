#pragma once

#include "bad_range.h"
#include "bad_length.h"
#include <iostream>

template <typename T>
class dynamic_array final {
public:
	dynamic_array() = default;
	explicit dynamic_array(int);
	dynamic_array(const dynamic_array &);
	dynamic_array(const dynamic_array &&) noexcept;
	~dynamic_array();
	dynamic_array &operator=(const dynamic_array &);
	dynamic_array &operator=(const dynamic_array &&) noexcept;
	T &operator[](int);
	const T &operator[](int) const;
	void erase();
	void reallocate(int);
	void resize(int);
	void insertBefore(T, int);
	void pushFront(T);
	void pushBack(T);
	void remove(int);
	int findFirst(T) const;
	int findLast(T) const;
	int getLength() const;

private:
	int length_{ 0 };
	T *data_{ nullptr };
};

template <typename T>
dynamic_array<T>::dynamic_array(const int length) : length_{ length } {
	if (length_ < 0) {
		throw bad_length{length};
	}

	if (length > 0) {
		data_ = new T[length];
	}
}

template <typename T>
dynamic_array<T>::dynamic_array(const dynamic_array &other) {
	reallocate(other.get_length());
	for (int index = 0; index < length_; ++index) {
		data_[index] = other.data_[index];
	}
}

template<typename T>
dynamic_array<T>::dynamic_array(const dynamic_array &&other) noexcept {
	data_ = other.data_;
	length_ = other.length_;
	other.data_ = nullptr;
	other.length_ = 0;
}

template <typename T>
dynamic_array<T>::~dynamic_array() {
	delete[] data_;
}

template <typename T>
dynamic_array<T> &dynamic_array<T>::operator=(const dynamic_array &other) {
	if (this == &other) {
		return *this; // self-assigning check
	}

	reallocate(other.get_length());
	for (int index = 0; index < length_; ++index) {
		data_[index] = other.data_[index];
	}

	return *this;
}

template<typename T>
dynamic_array<T> &dynamic_array<T>::operator=(const dynamic_array &&other) noexcept {
	data_ = other.data_;
	length_ = other.length_;
	other.data_ = nullptr;
	other.length_ = 0;

	return *this;
}

template <typename T>
T &dynamic_array<T>::operator[](const int index) {
	if (index < 0 || index >= length_) {
		throw bad_range{index};
	}
	return data_[index];
}

template <typename T>
const T &dynamic_array<T>::operator[](const int index) const {
	if (index < 0 || index >= length_) {
		throw bad_range{index};
	}
	return data_[index];
}

template <typename T>
void dynamic_array<T>::erase() {
	delete[] data_;
	length_ = 0;
	data_ = nullptr;
}

template <typename T>
void dynamic_array<T>::reallocate(const int new_length) {
	if (new_length <= 0) {
		throw bad_length{new_length};
	}

	erase();
	delete[] data_;
	data_ = new T[new_length];
	length_ = new_length;
}

template <typename T>
void dynamic_array<T>::resize(const int new_length) {
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

template <typename T>
void dynamic_array<T>::insertBefore(const T value, const int index) {
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

template <typename T>
void dynamic_array<T>::remove(const int index) {
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

template <typename T>
void dynamic_array<T>::pushFront(const T value) {
	insertBefore(value, 0);
}

template <typename T>
void dynamic_array<T>::pushBack(const T value) {
	insertBefore(value, length_);
}

template <typename T>
int dynamic_array<T>::findFirst(const T value) const {
	for (int i = 0; i < length_; ++i) {
		if (data_[i] == value) {
			return i;
		}
	}
}

template <typename T>
int dynamic_array<T>::findLast(const T value) const {
	for (int i = length_ - 1; i >= 0; --i) {
		if (data_[i] == value) {
			return i;
		}
	}
	return -1;
	}

template <typename T>
int dynamic_array<T>::getLength() const {
	return length_;
}
