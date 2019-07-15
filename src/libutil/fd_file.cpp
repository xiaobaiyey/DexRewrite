//
// Created by xiaobaiyey on 2019-07-15.
//

#include <util/fd_file.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <limits>
#include <base/macros.h>
#include <base/logging.h>
#include <fcntl.h>


FdFile::FdFile()
        : guard_state_(GuardState::kClosed), fd_(-1), auto_close_(true), read_only_mode_(false) {
}

FdFile::FdFile(int fd, bool check_usage)
        : guard_state_(check_usage ? GuardState::kBase : GuardState::kNoCheck),
          fd_(fd), auto_close_(true), read_only_mode_(false) {
}

FdFile::FdFile(int fd, const std::string &path, bool check_usage)
        : FdFile(fd, path, check_usage, false) {
}

FdFile::FdFile(int fd, const std::string &path, bool check_usage, bool read_only_mode)
        : guard_state_(check_usage ? GuardState::kBase : GuardState::kNoCheck),
          fd_(fd), file_path_(path), auto_close_(true), read_only_mode_(read_only_mode) {
}

FdFile::FdFile(const std::string &path, int flags, mode_t mode, bool check_usage)
        : fd_(-1), auto_close_(true) {
    Open(path, flags, mode);
    if (!check_usage || !IsOpened()) {
        guard_state_ = GuardState::kNoCheck;
    }
}

void FdFile::Destroy() {
    if (kCheckSafeUsage && (guard_state_ < GuardState::kNoCheck)) {
        if (guard_state_ < GuardState::kFlushed) {
            LOG(ERROR) << "File " << file_path_ << " wasn't explicitly flushed before destruction.";
        }
        if (guard_state_ < GuardState::kClosed) {
            LOG(ERROR) << "File " << file_path_ << " wasn't explicitly closed before destruction.";
        }
       // DCHECK_GE(guard_state_, GuardState::kClosed);
    }
    if (auto_close_ && fd_ != -1) {
        if (Close() != 0) {
            PLOG(WARNING) << "Failed to close file with fd=" << fd_ << " path=" << file_path_;
        }
    }
}

FdFile &FdFile::operator=(FdFile &&other) {
    if (this == &other) {
        return *this;
    }

    if (this->fd_ != other.fd_) {
        Destroy();  // Free old state.
    }

    guard_state_ = other.guard_state_;
    fd_ = other.fd_;
    file_path_ = std::move(other.file_path_);
    auto_close_ = other.auto_close_;
    read_only_mode_ = other.read_only_mode_;
    other.Release();  // Release other.

    return *this;
}

FdFile::~FdFile() {
    Destroy();
}

void FdFile::moveTo(GuardState target, GuardState warn_threshold, const char *warning) {
    if (kCheckSafeUsage) {
        if (guard_state_ < GuardState::kNoCheck) {
            if (warn_threshold < GuardState::kNoCheck && guard_state_ >= warn_threshold) {
                LOG(ERROR) << warning;
            }
            guard_state_ = target;
        }
    }
}

void FdFile::moveUp(GuardState target, const char *warning) {
    if (kCheckSafeUsage) {
        if (guard_state_ < GuardState::kNoCheck) {
            if (guard_state_ < target) {
                guard_state_ = target;
            } else if (target < guard_state_) {
                LOG(ERROR) << warning;
            }
        }
    }
}

void FdFile::DisableAutoClose() {
    auto_close_ = false;
}

bool FdFile::Open(const std::string &path, int flags) {
    return Open(path, flags, 0640);
}

bool FdFile::Open(const std::string &path, int flags, mode_t mode) {
    static_assert(O_RDONLY == 0, "Readonly flag has unexpected value.");
    DCHECK_EQ(fd_, -1) << path;
    read_only_mode_ = ((flags & O_ACCMODE) == O_RDONLY);
    fd_ = TEMP_FAILURE_RETRY(open(path.c_str(), flags, mode));
    if (fd_ == -1) {
        return false;
    }
    file_path_ = path;
    if (kCheckSafeUsage && (flags & (O_RDWR | O_CREAT | O_WRONLY)) != 0) {
        // Start in the base state (not flushed, not closed).
        guard_state_ = GuardState::kBase;
    } else {
        // We are not concerned with read-only files. In that case, proper flushing and closing is
        // not important.
        guard_state_ = GuardState::kNoCheck;
    }
    return true;
}

int FdFile::Close() {
    int result = close(fd_);

    // Test here, so the file is closed and not leaked.
    if (kCheckSafeUsage) {

        moveUp(GuardState::kClosed, nullptr);
    }


    if (result == -1) {
        return -errno;
    }


    fd_ = -1;
    file_path_ = "";
    return 0;
}

int FdFile::Flush() {
    DCHECK(!read_only_mode_);

#ifdef __linux__
    int rc = TEMP_FAILURE_RETRY(fdatasync(fd_));
#else
    int rc = TEMP_FAILURE_RETRY(fsync(fd_));
#endif

    moveUp(GuardState::kFlushed, "Flushing closed file.");
    if (rc == 0) {
        return 0;
    }

    // Don't report failure if we just tried to flush a pipe or socket.
    return errno == EINVAL ? 0 : -errno;
}

int64_t FdFile::Read(char *buf, int64_t byte_count, int64_t offset) const {
#ifdef __linux__
    int rc = TEMP_FAILURE_RETRY(pread64(fd_, buf, byte_count, offset));
#else
    int rc = TEMP_FAILURE_RETRY(pread(fd_, buf, byte_count, offset));
#endif
    return (rc == -1) ? -errno : rc;
}

int FdFile::SetLength(int64_t new_length) {
    DCHECK(!read_only_mode_);
#ifdef __linux__
    int rc = TEMP_FAILURE_RETRY(ftruncate64(fd_, new_length));
#else
    int rc = TEMP_FAILURE_RETRY(ftruncate(fd_, new_length));
#endif
    moveTo(GuardState::kBase, GuardState::kClosed, "Truncating closed file.");
    return (rc == -1) ? -errno : rc;
}

int64_t FdFile::GetLength() const {
    struct stat s;
    int rc = TEMP_FAILURE_RETRY(fstat(fd_, &s));
    return (rc == -1) ? -errno : s.st_size;
}

int64_t FdFile::Write(const char *buf, int64_t byte_count, int64_t offset) {
    DCHECK(!read_only_mode_);
#ifdef __linux__
    int rc = TEMP_FAILURE_RETRY(pwrite64(fd_, buf, byte_count, offset));
#else
    int rc = TEMP_FAILURE_RETRY(pwrite(fd_, buf, byte_count, offset));
#endif
    moveTo(GuardState::kBase, GuardState::kClosed, "Writing into closed file.");
    return (rc == -1) ? -errno : rc;
}

int FdFile::Fd() const {
    return fd_;
}

bool FdFile::ReadOnlyMode() const {
    return read_only_mode_;
}

bool FdFile::CheckUsage() const {
    return guard_state_ != GuardState::kNoCheck;
}

bool FdFile::IsOpened() const {
    return fd_ >= 0;
}

static ssize_t ReadIgnoreOffset(int fd, void *buf, size_t count, off_t offset) {
    DCHECK_EQ(offset, 0);
    return read(fd, buf, count);
}

template<ssize_t (*read_func)(int, void *, size_t, off_t)>
static bool ReadFullyGeneric(int fd, void *buffer, size_t byte_count, size_t offset) {
    char *ptr = static_cast<char *>(buffer);
    while (byte_count > 0) {
        ssize_t bytes_read = TEMP_FAILURE_RETRY(read_func(fd, ptr, byte_count, offset));
        if (bytes_read <= 0) {
            // 0: end of file
            // -1: error
            return false;
        }
        byte_count -= bytes_read;  // Reduce the number of remaining bytes.
        ptr += bytes_read;  // Move the buffer forward.
        offset += static_cast<size_t>(bytes_read);  // Move the offset forward.
    }
    return true;
}

bool FdFile::ReadFully(void *buffer, size_t byte_count) {
    return ReadFullyGeneric<ReadIgnoreOffset>(fd_, buffer, byte_count, 0);
}

bool FdFile::PreadFully(void *buffer, size_t byte_count, size_t offset) {
    return ReadFullyGeneric<pread>(fd_, buffer, byte_count, offset);
}

template<bool kUseOffset>
bool FdFile::WriteFullyGeneric(const void *buffer, size_t byte_count, size_t offset) {
    DCHECK(!read_only_mode_);
    moveTo(GuardState::kBase, GuardState::kClosed, "Writing into closed file.");
    DCHECK(kUseOffset || offset == 0u);
    const char *ptr = static_cast<const char *>(buffer);
    while (byte_count > 0) {
        ssize_t bytes_written = kUseOffset
                                ? TEMP_FAILURE_RETRY(pwrite(fd_, ptr, byte_count, offset))
                                : TEMP_FAILURE_RETRY(write(fd_, ptr, byte_count));
        if (bytes_written == -1) {
            return false;
        }
        byte_count -= bytes_written;  // Reduce the number of remaining bytes.
        ptr += bytes_written;  // Move the buffer forward.
        offset += static_cast<size_t>(bytes_written);
    }
    return true;
}

bool FdFile::PwriteFully(const void *buffer, size_t byte_count, size_t offset) {
    return WriteFullyGeneric<true>(buffer, byte_count, offset);
}

bool FdFile::WriteFully(const void *buffer, size_t byte_count) {
    return WriteFullyGeneric<false>(buffer, byte_count, 0u);
}

bool FdFile::Copy(FdFile *input_file, int64_t offset, int64_t size) {

    return true;
}

bool FdFile::Unlink() {
    if (file_path_.empty()) {
        return false;
    }

    // Try to figure out whether this file is still referring to the one on disk.
    bool is_current = false;
    {
        struct stat this_stat, current_stat;
        int cur_fd = TEMP_FAILURE_RETRY(open(file_path_.c_str(), O_RDONLY));
        if (cur_fd > 0) {
            // FdFile still exists.
            if (fstat(fd_, &this_stat) == 0 && fstat(cur_fd, &current_stat) == 0) {
                is_current = (this_stat.st_dev == current_stat.st_dev) &&
                             (this_stat.st_ino == current_stat.st_ino);
            }
            close(cur_fd);
        }
    }

    if (is_current) {
        unlink(file_path_.c_str());
    }

    return is_current;
}

bool FdFile::Erase(bool unlink) {
    DCHECK(!read_only_mode_);

    bool ret_result = true;
    if (unlink) {
        ret_result = Unlink();
    }

    int result;
    result = SetLength(0);
    result = Flush();
    result = Close();
    // Ignore the errors.

    return ret_result;
}

int FdFile::FlushCloseOrErase() {
    DCHECK(!read_only_mode_);
    int flush_result = Flush();
    if (flush_result != 0) {
        LOG(ERROR) << "CloseOrErase failed while flushing a file.";
        Erase();
        return flush_result;
    }
    int close_result = Close();
    if (close_result != 0) {
        LOG(ERROR) << "CloseOrErase failed while closing a file.";
        Erase();
        return close_result;
    }
    return 0;
}

int FdFile::FlushClose() {
    DCHECK(!read_only_mode_);
    int flush_result = Flush();
    if (flush_result != 0) {
        LOG(ERROR) << "FlushClose failed while flushing a file.";
    }
    int close_result = Close();
    if (close_result != 0) {
        LOG(ERROR) << "FlushClose failed while closing a file.";
    }
    return (flush_result != 0) ? flush_result : close_result;
}

void FdFile::MarkUnchecked() {
    guard_state_ = GuardState::kNoCheck;
}

bool FdFile::ClearContent() {
    DCHECK(!read_only_mode_);
    if (SetLength(0) < 0) {
        PLOG(ERROR) << "Failed to reset the length";
        return false;
    }
    return ResetOffset();
}

bool FdFile::ResetOffset() {
    DCHECK(!read_only_mode_);
    off_t rc = TEMP_FAILURE_RETRY(lseek(fd_, 0, SEEK_SET));
    if (rc == static_cast<off_t>(-1)) {
        PLOG(ERROR) << "Failed to reset the offset";
        return false;
    }
    return true;
}

int FdFile::Compare(FdFile *other) {
    int64_t length = GetLength();
    int64_t length2 = other->GetLength();
    if (length != length2) {
        return length < length2 ? -1 : 1;
    }
    static const size_t kBufferSize = 4096;
    std::unique_ptr<uint8_t[]> buffer1(new uint8_t[kBufferSize]);
    std::unique_ptr<uint8_t[]> buffer2(new uint8_t[kBufferSize]);
    size_t offset = 0;
    while (length > 0) {
        size_t len = std::min(kBufferSize, static_cast<size_t>(length));
        if (!PreadFully(&buffer1[0], len, offset)) {
            return -1;
        }
        if (!other->PreadFully(&buffer2[0], len, offset)) {
            return 1;
        }
        int result = memcmp(&buffer1[0], &buffer2[0], len);
        if (result != 0) {
            return result;
        }
        length -= len;
        offset += len;
    }
    return 0;
}


FdFile *FdFile::OpenFileForReading(const char *name) {
    return OpenFileWithFlags(name, O_RDONLY);
}

FdFile *FdFile::OpenFileReadWrite(const char *name) {
    return OpenFileWithFlags(name, O_RDWR);
}

static FdFile *CreateEmptyFile(const char *name, int extra_flags) {
    // In case the file exists, unlink it so we get a new file. This is necessary as the previous
    // file may be in use and must not be changed.
    unlink(name);

    return FdFile::OpenFileWithFlags(name, O_CREAT | extra_flags);
}

FdFile *FdFile::CreateEmptyFile(const char *name, int extra_flags) {
    // In case the file exists, unlink it so we get a new file. This is necessary as the previous
    // file may be in use and must not be changed.
    unlink(name);

    return FdFile::OpenFileWithFlags(name, O_CREAT | extra_flags);
}

FdFile *FdFile::CreateEmptyFile(const char *name) {
    return CreateEmptyFile(name, O_RDWR | O_TRUNC);
}

FdFile *FdFile::CreateEmptyFileWriteOnly(const char *name) {
    return CreateEmptyFile(name, O_WRONLY | O_TRUNC | O_NOFOLLOW | O_CLOEXEC);
}

FdFile *FdFile::OpenFileWithFlags(const char *name, int flags, bool auto_flush) {
    CHECK(name != nullptr);
    bool read_only = ((flags & O_ACCMODE) == O_RDONLY);
    bool check_usage = !read_only && auto_flush;
    std::unique_ptr<FdFile> file(new FdFile(name, flags, 0666, check_usage));
    if (!file->IsOpened()) {
        return nullptr;
    }
    return file.release();
}

bool FdFile::FileExists(const char *name, bool check_file_type) {
    struct stat st;
    if (stat(name, &st) == 0) {
        if (check_file_type) {
            return S_ISREG(st.st_mode);  // TODO: Deal with symlinks?
        } else {
            return true;
        }
    } else {
        return false;
    }
}

bool FdFile::DirectoryExists(const char *name) {
    struct stat st;
    if (stat(name, &st) == 0) {
        return S_ISDIR(st.st_mode);  // TODO: Deal with symlinks?
    } else {
        return false;
    }
}

int64_t FdFile::GetFileSizeBytes(const char *name) {
    struct stat st;
    if (stat(name, &st) == 0) {
        return st.st_size;  // TODO: Deal with symlinks? According to the documentation,
        // the st_size for a symlink is "the length of the pathname
        // it contains, without a terminating null byte."
    } else {
        return -1;
    }
}

void FdFile::DeleteFile(const char *name) {
    remove(name);
}