//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_FD_FILE_H
#define WAAPOLICY_FD_FILE_H

#include "random_access_file.h"
#include <base/macros.h>
#include <stdlib.h>
#include <string>

static constexpr bool kCheckSafeUsage = true;

class FdFile : public RandomAccessFile {
public:
    FdFile();

    // Creates an FdFile using the given file descriptor. Takes ownership of the
    // file descriptor. (Use DisableAutoClose to retain ownership.)
    FdFile(int fd, bool checkUsage);

    FdFile(int fd, const std::string &path, bool checkUsage);

    FdFile(int fd, const std::string &path, bool checkUsage, bool read_only_mode);

    FdFile(const std::string &path, int flags, bool checkUsage)
            : FdFile(path, flags, 0640, checkUsage) {}

    FdFile(const std::string &path, int flags, mode_t mode, bool checkUsage);

    // Move constructor.
    FdFile(FdFile &&other)
            : guard_state_(other.guard_state_),
              fd_(other.fd_),
              file_path_(std::move(other.file_path_)),
              auto_close_(other.auto_close_),
              read_only_mode_(other.read_only_mode_) {
        other.Release();  // Release the src.
    }

    // Move assignment operator.
    FdFile &operator=(FdFile &&other);

    // Release the file descriptor. This will make further accesses to this FdFile invalid. Disables
    // all further state checking.
    int Release() {
        int tmp_fd = fd_;
        fd_ = -1;
        guard_state_ = GuardState::kNoCheck;
        auto_close_ = false;
        return tmp_fd;
    }

    void Reset(int fd, bool check_usage) {
        if (fd_ != -1 && fd_ != fd) {
            Destroy();
        }
        fd_ = fd;
        if (check_usage) {
            guard_state_ = fd == -1 ? GuardState::kNoCheck : GuardState::kBase;
        } else {
            guard_state_ = GuardState::kNoCheck;
        }
        // Keep the auto_close_ state.
    }

    // Destroys an FdFile, closing the file descriptor if Close hasn't already
    // been called. (If you care about the return value of Close, call it
    // yourself; this is meant to handle failure cases and read-only accesses.
    // Note though that calling Close and checking its return value is still no
    // guarantee that data actually made it to stable storage.)
    virtual ~FdFile();

    // RandomAccessFile API.
    int Close() override;

    int64_t Read(char *buf, int64_t byte_count, int64_t offset) const override;

    int SetLength(int64_t new_length) override;

    int64_t GetLength() const override;

    int64_t Write(const char *buf, int64_t byte_count, int64_t offset) override;

    int Flush() override;

    // Short for SetLength(0); Flush(); Close();
    // If the file was opened with a path name and unlink = true, also calls Unlink() on the path.
    // Note that it is the the caller's responsibility to avoid races.
    bool Erase(bool unlink = false);

    // Call unlink() if the file was opened with a path, and if open() with the name shows that
    // the file descriptor of this file is still up-to-date. This is still racy, though, and it
    // is up to the caller to ensure correctness in a multi-process setup.
    bool Unlink();

    // Try to Flush(), then try to Close(); If either fails, call Erase().
    int FlushCloseOrErase();

    // Try to Flush and Close(). Attempts both, but returns the first error.
    int FlushClose();

    // Bonus API.
    int Fd() const;

    bool ReadOnlyMode() const;

    bool CheckUsage() const;

    bool IsOpened() const;

    const std::string &GetPath() const {
        return file_path_;
    }

    void DisableAutoClose();

    bool ReadFully(void *buffer, size_t byte_count);

    bool PreadFully(void *buffer, size_t byte_count, size_t offset);

    bool WriteFully(const void *buffer, size_t byte_count);

    bool PwriteFully(const void *buffer, size_t byte_count, size_t offset);

    // Copy data from another file.
    bool Copy(FdFile *input_file, int64_t offset, int64_t size);

    // Clears the file content and resets the file offset to 0.
    // Returns true upon success, false otherwise.
    bool ClearContent();

    // Resets the file offset to the beginning of the file.
    bool ResetOffset();

    // This enum is public so that we can define the << operator over it.
    enum class GuardState {
        kBase,           // Base, file has not been flushed or closed.
        kFlushed,        // FdFile has been flushed, but not closed.
        kClosed,         // FdFile has been flushed and closed.
        kNoCheck         // Do not check for the current file instance.
    };

    // WARNING: Only use this when you know what you're doing!
    void MarkUnchecked();

    // Compare against another file. Returns 0 if the files are equivalent, otherwise returns -1 or 1
    // depending on if the lenghts are different. If the lengths are the same, the function returns
    // the difference of the first byte that differs.
    int Compare(FdFile *other);

protected:
    // If the guard state indicates checking (!=kNoCheck), go to the target state "target". Print the
    // given warning if the current state is or exceeds warn_threshold.
    void moveTo(GuardState target, GuardState warn_threshold, const char *warning);

    // If the guard state indicates checking (<kNoCheck), and is below the target state "target", go
    // to "target." If the current state is higher (excluding kNoCheck) than the trg state, print the
    // warning.
    void moveUp(GuardState target, const char *warning);

    // Forcefully sets the state to the given one. This can overwrite kNoCheck.
    void resetGuard(GuardState new_state) {
        if (kCheckSafeUsage) {
            guard_state_ = new_state;
        }
    }

    GuardState guard_state_;

    // Opens file 'file_path' using 'flags' and 'mode'.
    bool Open(const std::string &file_path, int flags);

    bool Open(const std::string &file_path, int flags, mode_t mode);

private:
    template<bool kUseOffset>
    bool WriteFullyGeneric(const void *buffer, size_t byte_count, size_t offset);

    void Destroy();  // For ~FdFile and operator=(&&).

    int fd_;
    std::string file_path_;
    bool auto_close_;
    bool read_only_mode_;


public:
    // Open an existing file with read only access.
    static FdFile *OpenFileForReading(const char *name);

    // Open an existing file with read/write access.
    static FdFile *OpenFileReadWrite(const char *name);

    // Create an empty file with read/write access. This is a *new* file, that is, if the file
    // already exists, it is *not*z overwritten, but unlinked, and a new inode will be used.
    static FdFile *CreateEmptyFile(const char *name);

    static FdFile *CreateEmptyFile(const char *name, int extra_flags);

    // Create an empty file with write access. This is a *new* file, that is, if the file
    // already exists, it is *not* overwritten, but unlinked, and a new inode will be used.
    static FdFile *CreateEmptyFileWriteOnly(const char *name);

    // Open a file with the specified open(2) flags.
    static FdFile *OpenFileWithFlags(const char *name, int flags, bool auto_flush = true);

    // Check if a file exists.
    static bool FileExists(const char *name, bool check_file_type = true);

    // Check if a directory exists.
    static bool DirectoryExists(const char *name);

    // Get the size of a file (or -1 if it does not exist).
    static int64_t GetFileSizeBytes(const char *name);

    static void DeleteFile(const char *name);

    DISALLOW_COPY_AND_ASSIGN(FdFile);
};





#endif //WAAPOLICY_FD_FILE_H
