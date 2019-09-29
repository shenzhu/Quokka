#include <cassert>

#include "buffer.h"

namespace Quokka {

const std::size_t Buffer::kMaxBufferSize = std::numeric_limits<std::size_t>::max() / 2;
const std::size_t Buffer::kHighWaterMark = 1 * 1024;
const std::size_t Buffer::kDefaultSize = 256;

inline static std::size_t roundUp2Power(std::size_t size) {
	if (size == 0) {
		return 0;
	}

	std::size_t roundUp = 1;
	while (roundUp < size) {
		roundUp *= 2;
	}

	return roundUp;
}

std::size_t Buffer::pushData(const void* data, std::size_t size) {
	std::size_t bytes = pushDataAt(data, size);
	produce(bytes);

	assert(bytes == size);

	return bytes;
}

std::size_t Buffer::pushDataAt(const void* data, std::size_t size, std::size_t offset) {
	if (!data || size == 0) {
		return 0;
	}

	if (readableSize() + size + offset >= kMaxBufferSize) {
		return 0;
	}

	assureSpace(size + offset);

	assert(size + offset <= writableSize());

	memcpy(&buffer_[writePos_ + offset], data, size);
	return size;
}

void Buffer::consume(std::size_t bytes) {
	assert(readPos_ + bytes <= writePos_);

	readPos_ += bytes;
	if (isEmpty()) {
		clear();
	}
}

std::size_t Buffer::peekDataAt(void* buffer, std::size_t size, std::size_t offset) {
	const std::size_t dataSize = readableSize();
	if (!buffer || size == 0 || dataSize <= offset) {
		return 0;
	}

	if (size + offset > dataSize) {
		size = dataSize - offset;
	}

	memcpy(buffer, &buffer_[readPos_ + offset], size);
	return size;
}

std::size_t Buffer::popData(void* buffer, std::size_t size) {
	std::size_t bytes = peekDataAt(buffer, size);
	consume(bytes);

	return bytes;
}

void Buffer::assureSpace(std::size_t needSize) {
	if (writableSize() >= needSize) {
		return;
	}

	const std::size_t dataSize = readableSize();
	const std::size_t oldCap = capacity_;

	// writableSize() + readPos_为还可以用的内存容量
	while (writableSize() + readPos_ < needSize) {
		if (capacity_ < kDefaultSize) {
			capacity_ = kDefaultSize;
		}
		else if (capacity_ < kMaxBufferSize) {
			const auto newCapacity = roundUp2Power(capacity_);
			if (capacity_ < newCapacity) {
				capacity_ = newCapacity;
			}
			else {
				capacity_ = 3 * newCapacity / 2;
			}
		}
		else {
			// Error, needSize too big
			assert(false);
		}
	}

	if (oldCap < capacity_) {
		std::unique_ptr<char[]> temp(new char[capacity_]);

		if (dataSize != 0) {
			memcpy(&temp[0], &buffer_[readPos_], dataSize);
		}

		buffer_.swap(temp);
	}
	else {
		assert(readPos_ > 0);
		memmove(&buffer_[0], &buffer_[readPos_], dataSize);
	}

	readPos_ = 0;
	writePos_ = dataSize;
}

void Buffer::shrink() {
	if (isEmpty()) {
		/*
		这里的isEmpty并不是表示当前的这个buffer是完全空的
		而是表明当前已经没有用可读的data了
		从isEmpty()的实现也可以看出，它是计算writePos_ - readPos_
		也就是说，如果当前的buffer里面已经没有可读的data了
		就可以把之前的清空，因为之前的data也已经读过了
		*/
		if (capacity_ > 8 * 1024) {
			clear();
			capacity_ = 0;
			buffer_.reset();
		}

		return;
	}

	std::size_t oldCap = capacity_;
	std::size_t dataSize = readableSize();
	if (dataSize > oldCap / 4) {
		return;
	}

	std::size_t newCap = roundUp2Power(dataSize);

	std::unique_ptr<char[]> temp(new char[newCap]);
	memcpy(&temp[0], &buffer_[readPos_], dataSize);
	buffer_.swap(temp);
	capacity_ = newCap;

	readPos_ = 0;
	writePos_ = dataSize;
}

void Buffer::clear() {
	readPos_ = writePos_ = 0;
}

void Buffer::swap(Buffer& buffer) {
	/*
	前三个使用的是std::swap
	Swaps the values a and b.

	最后一个用的是std::unique_ptr::swap
	Swaps the managed objects and associated deleters of *this and another unique_ptr
	*/
	std::swap(readPos_, buffer.readPos_);
	std::swap(writePos_, buffer.writePos_);
	std::swap(capacity_, buffer.capacity_);
	buffer_.swap(buffer.buffer_);
}

Buffer::Buffer(Buffer&& other) {
	_moveFrom(std::move(other));
}

Buffer& Buffer::operator=(Buffer&& other) {
	return _moveFrom(std::move(other));
}

Buffer& Buffer::_moveFrom(Buffer&& other) {
	if (this != &other) {
		this->readPos_ = other.readPos_;
		this->writePos_ = other.writePos_;
		this->capacity_ = other.capacity_;
		this->buffer_ = std::move(other.buffer_);

		other.clear();
		other.shrink();
	}

	return *this;
}

}  // namespace Quokka