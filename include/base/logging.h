#ifndef _waa_BASE_LOGGING_H_
#define _waa_BASE_LOGGING_H_

#include <string>
#include <sstream>

#include "base/macros.h"

#ifdef __ANDROID__

#include <android/log.h>

#endif


#define CHECK(x) \
  if (UNLIKELY(!(x))) /*  NOLINT*/ \
    LogMessageFatal(FATAL,__FILE__, __LINE__).stream() \
        << "Check failed: " #x << " "

#define CHECK_OP(LHS, RHS, OP) \
  for (auto _values = MakeEagerEvaluator(LHS, RHS); \
       UNLIKELY(!(_values.lhs OP _values.rhs)); /* empty */) \
        LogMessage(ERROR,__FILE__, __LINE__).stream() \
        << "Check failed: " << #LHS << " " << #OP << " " << #RHS \
        << " (" #LHS "=" << _values.lhs << ", " #RHS "=" << _values.rhs << ") "

#define CHECK_EQ(x, y) CHECK_OP(x, y, ==)
#define CHECK_NE(x, y) CHECK_OP(x, y, !=)
#define CHECK_LE(x, y) CHECK_OP(x, y, <=)
#define CHECK_LT(x, y) CHECK_OP(x, y, <)
#define CHECK_GE(x, y) CHECK_OP(x, y, >=)
#define CHECK_GT(x, y) CHECK_OP(x, y, >)

#define CHECK_STROP(s1, s2, sense) \
  if (UNLIKELY((strcmp(s1, s2) == 0) != sense)) \
    LOG(FATAL) << "Check failed: " \
               << "\"" << s1 << "\"" \
               << (sense ? " == " : " != ") \
               << "\"" << s2 << "\""

#define CHECK_STREQ(s1, s2) CHECK_STROP(s1, s2, true)
#define CHECK_STRNE(s1, s2) CHECK_STROP(s1, s2, false)

#define CHECK_CONSTEXPR(x, out, dummy) \
  (UNLIKELY(!(x))) ? (LOG(FATAL) << "Check failed: " << #x out, dummy) :

#ifndef NDEBUG

#define DCHECK(x) CHECK(x)
#define DCHECK_EQ(x, y) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) CHECK_NE(x, y)
#define DCHECK_LE(x, y) CHECK_LE(x, y)
#define DCHECK_LT(x, y) CHECK_LT(x, y)
#define DCHECK_GE(x, y) CHECK_GE(x, y)
#define DCHECK_GT(x, y) CHECK_GT(x, y)
#define DCHECK_STREQ(s1, s2) CHECK_STREQ(s1, s2)
#define DCHECK_STRNE(s1, s2) CHECK_STRNE(s1, s2)
#define DCHECK_CONSTEXPR(x, out, dummy) CHECK_CONSTEXPR(x, out, dummy)

#else  // NDEBUG

#define DCHECK(condition) \
  while (false) \
    CHECK(condition)

#define DCHECK_EQ(val1, val2) \
  while (false) \
    CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
  while (false) \
    CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
  while (false) \
    CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
  while (false) \
    CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
  while (false) \
    CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
  while (false) \
    CHECK_GT(val1, val2)

#define DCHECK_STREQ(str1, str2) \
  while (false) \
    CHECK_STREQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
  while (false) \
    CHECK_STRNE(str1, str2)

#define DCHECK_CONSTEXPR(x, out, dummy) \
  (false && (x)) ? (dummy) :

#endif


#define LOG_INFO LogMessage(INFO,__FILE__, __LINE__)
#define LOG_WARNING LogMessage(WARN,__FILE__, __LINE__)
#define LOG_ERROR LogMessage(ERROR,__FILE__, __LINE__)
#define LOG_FATAL LogMessageFatal(FATAL,__FILE__, __LINE__)
#define LOG_QFATAL LOG_FATAL

#ifdef NDEBUG
#define LOG_DFATAL LOG_ERROR
#else
#define LOG_DFATAL LOG_FATAL
#endif

#define LOG(severity) LOG_ ## severity.stream()
#define PLOG(severity) LOG_ ## severity.stream()

#define VLOG(x) if ((x) > 0) {} else LOG_INFO.stream() // NOLINT


template<typename LHS, typename RHS>
struct EagerEvaluator {
    EagerEvaluator(LHS lhs, RHS rhs) : lhs(lhs), rhs(rhs) {}

    LHS lhs;
    RHS rhs;
};

template<typename LHS, typename RHS>
EagerEvaluator<LHS, RHS> MakeEagerEvaluator(LHS lhs, RHS rhs) {
    return EagerEvaluator<LHS, RHS>(lhs, rhs);
}

#define EAGER_PTR_EVALUATOR(T1, T2) \
  template <> struct EagerEvaluator<T1, T2> { \
    EagerEvaluator(T1 lhs, T2 rhs) \
        : lhs(reinterpret_cast<const void*>(lhs)), \
          rhs(reinterpret_cast<const void*>(rhs)) { } \
    const void* lhs; \
    const void* rhs; \
  }

EAGER_PTR_EVALUATOR(const char*, const char*);

EAGER_PTR_EVALUATOR(const char*, char*);

EAGER_PTR_EVALUATOR(char*, const char*);

EAGER_PTR_EVALUATOR(char*, char*);

EAGER_PTR_EVALUATOR(const unsigned char*, const unsigned char*);

EAGER_PTR_EVALUATOR(const unsigned char*, unsigned char*);

EAGER_PTR_EVALUATOR(unsigned char*, const unsigned char*);

EAGER_PTR_EVALUATOR(unsigned char*, unsigned char*);

EAGER_PTR_EVALUATOR(const signed char*, const signed char*);

EAGER_PTR_EVALUATOR(const signed char*, signed char*);

EAGER_PTR_EVALUATOR(signed char*, const signed char*);

EAGER_PTR_EVALUATOR(signed char*, signed char*);


typedef enum LogPriority {
    /** For internal use only.  */
            UNKNOWN = 0,
    /** The default priority, for internal use only.  */
            DEFAULT, /* only for SetMinPriority() */
    /** Verbose logging. Should typically be disabled for a release apk. */
            VERBOSE,
    /** Debug logging. Should typically be disabled for a release apk. */
            DEBUG,
    /** Informational logging. Should typically be disabled for a release apk. */
            INFO,
    /** Warning logging. For use with recoverable failures. */
            WARN,
    /** Error logging. For use with unrecoverable failures. */
            ERROR,
    /** Fatal logging. For use when aborting. */
            FATAL,
    /** For internal use only.  */
            SILENT, /* only for SetMinPriority(); must be last */
} LogPriority;


class LogMessage {

public:
    LogMessage(LogPriority logPriority, const char *file, int line)
            : flushed_(false), logPriority_(logPriority) {
        switch (logPriority) {
            case UNKNOWN:
                str_ << "[UNKNOWN]";
                break;
            case DEFAULT:
                str_ << "[DEFAULT]";
                break;
            case VERBOSE:
                str_ << "[VERBOSE]";
                break;
            case DEBUG:
                str_ << "[DEBUG]";
                break;
            case INFO:
                str_ << "[INFO]";
                break;
            case WARN:
                str_ << "[WARN]";
                break;
            case ERROR:
                str_ << "[ERROR]";
                break;
            case SILENT:
                str_ << "[SILENT]";
                break;
            default:
                break;
        }
    }

    LogMessage(const LogMessage &) = delete;

    LogMessage &operator=(const LogMessage &) = delete;

    void Flush() {
        std::string s = str_.str();
        size_t n = s.size();
#ifdef __ANDROID__
        __android_log_write(ANDROID_LOG_ERROR, "Waa", s.c_str());
#else
        fwrite(s.data(), 1, n, stderr);
#endif
        flushed_ = true;
    }

    virtual ~LogMessage() {
        if (!flushed_) {
            Flush();
        }
    }

    std::ostream &stream() { return str_; }

private:
    bool flushed_;
    LogPriority logPriority_;
    std::ostringstream str_;
};


class LogMessageFatal : public LogMessage {

public:
    LogMessageFatal(LogPriority logPriority, const char *file, int line)
            : LogMessage(logPriority, file, line) {
    }

    LogMessageFatal(const LogMessageFatal &) = delete;

    LogMessageFatal &operator=(const LogMessageFatal &) = delete;

    NO_RETURN ~LogMessageFatal() override {
        Flush();
        abort();
    }
};


#if defined(__ANDROID__)

#include <android/log.h>

#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, __FUNCTION__, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, __FUNCTION__, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, __FUNCTION__,__VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, __FUNCTION__,__VA_ARGS__))
#else

#include <stdint.h>

typedef enum android_LogPriority {
    /** For internal use only.  */
            ANDROID_LOG_UNKNOWN = 0,
    /** The default priority, for internal use only.  */
            ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
    /** Verbose logging. Should typically be disabled for a release apk. */
            ANDROID_LOG_VERBOSE,
    /** Debug logging. Should typically be disabled for a release apk. */
            ANDROID_LOG_DEBUG,
    /** Informational logging. Should typically be disabled for a release apk. */
            ANDROID_LOG_INFO,
    /** Warning logging. For use with recoverable failures. */
            ANDROID_LOG_WARN,
    /** Error logging. For use with unrecoverable failures. */
            ANDROID_LOG_ERROR,
    /** Fatal logging. For use when aborting. */
            ANDROID_LOG_FATAL,
    /** For internal use only.  */
            ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
} android_LogPriority;

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
static char sprint_buf[1024];
__attribute__((__format__(printf, 3, 4)))
inline int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsprintf(sprint_buf, fmt, args);
    va_end(args);
    write(1, sprint_buf, n);
    write(1, "\n", 1);
    fsync(1);
    return n;
}

#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_INFO, __FUNCTION__, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, __FUNCTION__, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, __FUNCTION__,__VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, __FUNCTION__,__VA_ARGS__))


/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#endif


#endif  // _waa_BASE_LOGGING_H_
