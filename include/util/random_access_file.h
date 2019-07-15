//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_RANDOM_ACCESS_FILE_H
#define WAAPOLICY_RANDOM_ACCESS_FILE_H

#include <stdint.h>

class RandomAccessFile {
public:
    virtual ~RandomAccessFile() { }

    virtual int Close() = 0;

    // Reads 'byte_count' bytes into 'buf' starting at offset 'offset' in the
    // file. Returns the number of bytes actually read.
    virtual int64_t Read(char* buf, int64_t byte_count, int64_t offset) const = 0;

    // Sets the length of the file to 'new_length'. If this is smaller than the
    // file's current extent, data is discarded. If this is greater than the
    // file's current extent, it is as if a write of the relevant number of zero
    // bytes occurred. Returns 0 on success.
    virtual int SetLength(int64_t new_length) = 0;

    // Returns the current size of this file.
    virtual int64_t GetLength() const = 0;

    // Writes 'byte_count' bytes from 'buf' starting at offset 'offset' in the
    // file. Zero-byte writes are acceptable, and writes past the end are as if
    // a write of the relevant number of zero bytes also occurred. Returns the
    // number of bytes actually written.
    virtual int64_t Write(const char* buf, int64_t byte_count, int64_t offset) = 0;

    // Flushes file data.
    virtual int Flush() = 0;
};


#endif //WAAPOLICY_RANDOM_ACCESS_FILE_H
