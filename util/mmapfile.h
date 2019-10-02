#pragma once

#include <string>

namespace Quokka {

class OMmapFile {
public:

    OMmapFile();
    ~OMmapFile();

    void close();

    void truncate(std::size_t size);

    bool isOpen() const;

private:
    void _assureSpace(std::size_t size);
    bool _mapWriteOnly();
    void _extendFileSize(std::size_t size);

    int file_;
    char* memory_;
    std::size_t offset_;
    std::size_t size_;
    std::size_t syncPos_;
};

}  // namespace Quokka