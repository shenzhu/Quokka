#pragma once

#include <cstring>
#include <memory>
#include <list>

namespace Quokka {

class Buffer {
public:

	Buffer() :
		readPos_(0),
		writePos_(0),
		capacity_(0) {
	}

	Buffer(const void* data, std::size_t size) :
		readPos_(0),
		writePos_(0),
		capacity_(0) {
		pushData(data, size);
	}

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	Buffer(Buffer&&);
	Buffer& operator=(Buffer&&);

	std::size_t pushDataAt(const void* data, std::size_t size, std::size_t offset = 0);
	std::size_t pushData(const void* data, std::size_t size);
	void produce(std::size_t bytes) {
		writePos_ += bytes;
	}

	std::size_t peekDataAt(void* buffer, std::size_t size, std::size_t offset = 0);
	std::size_t popData(void* buffer, std::size_t size);
	void consume(std::size_t bytes);

	char* readAddr() {
		return &buffer_[readPos_];
	}

	char* writeAddr() {
		return &buffer_[writePos_];
	}

	std::size_t readableSize() const {
		return writePos_ - readPos_;
	}

	std::size_t writableSize() const {
		return capacity_ - writePos_;
	}
	
	std::size_t capacity() const {
		return capacity_;
	}
	
	bool isEmpty() const {
		return readableSize() == 0;
	}

	void clear();
	void shrink();
	void swap(Buffer& buffer);

	void assureSpace(std::size_t size);

	static const std::size_t kMaxBufferSize;
	static const std::size_t kHighWaterMark;
	static const std::size_t kDefaultSize;

private:

	Buffer& _moveFrom(Buffer&&);

	std::size_t readPos_;
	std::size_t writePos_;
	std::size_t capacity_;
	std::unique_ptr<char[]> buffer_;
};

struct BufferVector {
	static constexpr int kMinSize = 1024;

	BufferVector() {}

	BufferVector(Buffer&& first) {
		push(std::move(first));
	}

	bool empty() const {
		return buffers.empty();
	}

	std::size_t totalByteSize() const {
		return totalBytes;
	}

	void clear() {
		buffers.clear();
		totalBytes = 0;
	}

	void push(Buffer&& buffer) {
		totalBytes += buffer.readableSize();
		if (_shouldMerge()) {
			auto& last = buffers.back();
			last.pushData(buffer.readAddr(), buffer.readableSize());
		}
		else {
			buffers.push_back(std::move(buffer));
		}
	}

	void push(const void* data, std::size_t size) {
		totalBytes += size;
		/*
		如果最后一个Buffer的size比较小，就把要加的data放到其中去
		如果并没有比较小，就在后面添加一个新的Buffer
		*/
		if (_shouldMerge()) {
			auto& last = buffers.back();
			last.pushData(data, size);
		}
		else {
			buffers.push_back(Buffer(data, size));
		}
	}

	void pop() {
		totalBytes -= buffers.front().readableSize();
		buffers.pop_front();
	}

	std::list<Buffer>::iterator begin() {
		return buffers.begin();
	}

	std::list<Buffer>::iterator end() {
		return buffers.end();
	}

	std::list<Buffer>::const_iterator begin() const {
		return buffers.begin();
	}

	std::list<Buffer>::const_iterator end() const {
		return buffers.end();
	}

	std::list<Buffer>::const_iterator cbegin() const {
		return buffers.cbegin();
	}

	std::list<Buffer>::const_iterator cend() const {
		return buffers.cend();
	}

	std::list<Buffer> buffers;
	std::size_t totalBytes{ 0 };

private:
	
	/*
	这个函数并不是check当前list中的所有buffer，
	而是只检查list中的最后一个buffer，如果最后一个buffer的可读区域(也就是被写入但是还没有读的区域)
	比较小，那么就返回true，表明最后一个buffer可以被merge
	*/
	bool _shouldMerge() const {
		if (buffers.empty()) {
			return false;
		}
		else {
			const auto& last = buffers.back();
			return last.readableSize() < kMinSize;
		}
	}
};

struct Slice {
	const void* data;
	std::size_t len;


	explicit Slice(const void* d = nullptr, std::size_t l = 0) :
		data(d),
		len(l) {
	}
};

struct SliceVector {

	std::list<Slice>::iterator begin() {
		return slices.begin();
	}

	std::list<Slice>::iterator end() {
		return slices.end();
	}

	std::list<Slice>::const_iterator begin() const {
		return slices.begin();
	}

	std::list<Slice>::const_iterator end() const {
		return slices.end();
	}

	std::list<Slice>::const_iterator cbegin() const {
		return slices.cbegin();
	}

	std::list<Slice>::const_iterator cend() const {
		return slices.cend();
	}

	bool empty() const {
		return slices.empty();
	}

	void pushBack(const void* data, std::size_t len) {
		slices.push_back(Slice(data, len));
	}

private:

	std::list<Slice> slices;
};

}  // namespace Quokka