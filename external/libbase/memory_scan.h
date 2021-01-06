//
// Created by xiaobaiyey on 2019-06-08.
//

#ifndef STRSPLT_MEMORYSCAN_H
#define STRSPLT_MEMORYSCAN_H

#include <string>
#include <vector>
#include <functional>
#include <stdint.h>
#include <sys/types.h>
class MemoryScan {
public:

    explicit MemoryScan(std::string pattern_str);

    void memoryScanSync(void *base, size_t len, std::function<bool(uint8_t *address)> func);

    void memoryScanSync(void *base, void *end, std::function<bool(uint8_t *address)> func);

    uint8_t *memoryScanOnce(void *base, void *end);


    void memoryScanAsyn(void *base, size_t len, std::function<bool(uint8_t *address)> func);

    void memoryScanAsyn(void *base, void *end, std::function<bool(uint8_t *address)> func);

    ~MemoryScan();

private:
    enum MatchType {
        MATCH_EXACT,
        MATCH_WILDCARD,
        MATCH_MASK
    };
    typedef struct MatchToken {
        MatchType type;
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> masks;
        uint offset;
    public:
        ~MatchToken();
    } MatchToken;

    typedef struct MatchPattern {
        std::vector<MatchToken *> tokens;
        uint size;
        size_t byte_len;
    } MatchPattern;

    typedef struct Args {
        void *base_;
        size_t len;
        std::function<bool(uint8_t *address)> func;
        MemoryScan::MatchPattern *pattern_;
    } Args;

private:

    static bool memory_scan(void *base, size_t len, std::function<bool(uint8_t *address)> func, MatchPattern *pattern_);

    static void *memory_scan_asyn(void *arg);

    static bool match_before(size_t index, uint8_t *&data, MatchPattern *pattern_);

    static bool match_after(size_t index, uint8_t *&data, MatchPattern *pattern_);

    static MatchToken *match_pattern_get_longest_token(const MatchPattern *self, MatchType type);

    static bool memcmp_mask(MatchToken *matchToken, uint8_t *temp);

    static MatchPattern *copy_pattern(MatchPattern *pattern);

private:
    std::vector<std::string> string_split(std::string string, std::string delimiter, int max_tokens);

    void match_pattern_new_from_string(std::string str);

    int ascii_xdigit_value(char c);

    MatchPattern *match_token_cmp(MatchPattern *pattern);

    MatchToken *match_pattern_push_token(MatchPattern *self, MatchType type);


private:
    MatchPattern *pattern_;
    std::string pattern_str_;


};


#endif //STRSPLT_MEMORYSCAN_H
