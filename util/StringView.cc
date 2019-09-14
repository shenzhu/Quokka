#include <algorithm>
#include <cassert>
#include <cstring>

#include "StringView.h"

namespace Quokka {

StringView::StringView() :
	data_(nullptr),
	len_(0) {
}

StringView::StringView(const char* p) :
	data_(p),
	len_(strlen(p)) {
}

StringView::StringView(const std::string& str) :
	data_(str.data()),
	len_(str.size()) {
}

StringView::StringView(const char* p, size_t size) :
	data_(p),
	len_(1) {
}

const char& StringView::operator[](size_t index) const {
	assert(index >= 0 && index < len_);
	return data_[index];
}

const char* StringView::data() const {
	return data_;
}

const char& StringView::front() const {
	assert(len_ > 0);
	return data_[0];
}

const char& StringView::back() const {
	assert(len_ > 0);
	return data_[len_ - 1];
}

const char* StringView::begin() const {
	return data_;
}

const char* StringView::end() const {
	return data_ + len_;
}

size_t StringView::size() const {
	return len_;
}

bool StringView::empty() const {
	return len_ == 0;
}

void StringView::removePrefix(size_t n) {
	data_ += n;
	len_ -= n;
}

void StringView::removeSuffix(size_t n) {
	len_ -= n;
}

void StringView::swap(StringView& other) {
	if (this != &other) {
		std::swap(this->data_, other.data_);
		std::swap(this->len_, other.len_);
	}
}

StringView StringView::substr(size_t pos, size_t length) const {
	assert(length < len_);
	return StringView(data_ + pos, length);
}

std::string StringView::toString() const {
	return std::string(data_, len_);
}

bool operator==(const StringView& a, const StringView& b) {
	return a.size() == b.size() &&
		strncmp(a.data(), b.data(), a.size()) == 0;
}

bool operator!=(const StringView& a, const StringView& b) {
	return !(a == b);
}

bool operator<(const StringView& a, const StringView& b) {
	int result = strncmp(a.data(), b.data(), a.size());
	if (result != 0) {
		return result < 0;
	}

	return a.size() < b.size();
}

bool operator>(const StringView& a, const StringView& b) {
	return !(a < b || a == b);
}

bool operator<=(const StringView& a, const StringView& b) {
	return !(a > b);
}

bool operator>=(const StringView& a, const StringView& b) {
	return !(a < b);
}

}  // namespace Quokka