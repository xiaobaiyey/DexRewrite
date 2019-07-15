/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:22 PM
* @ class describe
*/

#ifndef UNPACKER_COLLECTIONVECTOR_H
#define UNPACKER_COLLECTIONVECTOR_H

#include <vector>
#include "dex_ir_collectionbase.h"
#include <cxxabi.h>
#include <base/macros.h>
#include <base/logging.h>

namespace dex_ir {
    template<class T>
    class CollectionVector : public CollectionBase<T> {
    public:
        using Vector = std::vector<std::unique_ptr<T>>;

        CollectionVector() {
            collection_.clear();
        };

        uint32_t Size() const { return collection_.size(); }

        Vector &Collection() { return collection_; }

        const Vector &Collection() const { return collection_; }

        // Sort the vector by copying pointers over.
        template<typename MapType>
        void SortByMapOrder(const MapType &map) {
            auto it = map.begin();
            CHECK_EQ(map.size(), Size());
            for (size_t i = 0; i < Size(); ++i) {
                // There are times when the array will temporarily contain the same pointer twice, doing the
                // release here sure there is no double free errors.
                Collection()[i].release();
                Collection()[i].reset(it->second);
                ++it;
            }
        }

        ~CollectionVector() {
            const char *symbol = typeid(T).name();
            int status;
            char *demangled = abi::__cxa_demangle(symbol, 0, 0, &status);
#if defined(__arm__) || defined(__i386__) || defined(__mips__)
            LOGI("[-] start free %s 0x%x", demangled, collection_.size());
#elif defined(__aarch64__) || defined(__x86_64__) || defined(__mips64__)
            LOGI("[-] start free %s 0x%lx", demangled, collection_.size());;
#endif

        }

    protected:
        Vector collection_;

        void AddItem(T *object) {
            collection_.push_back(std::unique_ptr<T>(object));
        }

    private:
        friend class Collections;
        DISALLOW_COPY_AND_ASSIGN(CollectionVector);
    };

}
#endif //UNPACKER_COLLECTIONVECTOR_H
