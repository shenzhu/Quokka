#pragma once

#include <ostream>
#include <string>

namespace Quokka {

class StringView {
public:
	
	StringView();
	StringView(const char* p);
	StringView(const std::string& str);
	StringView(const char* ch, size_t size);
	StringView(const StringView& rhs) = default;

	const char& operator[](size_t index) const;

	const char* data() const;
	const char& front() const;
	const char& back() const;

	const char* begin() const;
	const char* end() const;

	size_t size() const;
	bool empty() const;

	void removePrefix(size_t n);
	void removeSuffix(size_t n);

	void swap(StringView& other);
	StringView substr(size_t pos, size_t length) const;

	std::string toString() const;

private:

	const char* data_;
	size_t len_;
};

bool operator==(const StringView& a, const StringView& b);
bool operator!=(const StringView& a, const StringView& b);
bool operator<(const StringView& a, const StringView& b);
bool operator>(const StringView& a, const StringView& b);
bool operator<=(const StringView& a, const StringView& b);
bool operator>=(const StringView& a, const StringView& b);

inline std::ostream& operator<<(std::ostream& os, const StringView& sv) {
	return os << sv.data();
}

}  // namespace Quokka

namespace std {
template<>
struct hash<Quokka::StringView> {
	size_t operator()(const Quokka::StringView& sv) const noexcept {
		size_t result = 0;
		for (const auto& ch : sv) {
			result = (result * 131) + ch;
		}
		return result;
	}
};
}