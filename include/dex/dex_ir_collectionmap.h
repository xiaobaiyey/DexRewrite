/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:24 PM
* @ class describe
*/

#ifndef UNPACKER_COLLECTIONMAP_H
#define UNPACKER_COLLECTIONMAP_H

#include <map>
#include "dex_ir_collectionbase.h"
#include <base/macros.h>
#include <base/logging.h>


namespace dex_ir {

    template<class T>
    class CollectionMap : public CollectionBase<T> {
    public:
        CollectionMap() = default;

        // Returns the existing item if it is already inserted, null otherwise.
        T *GetExistingObject(uint32_t offset) {
            auto it = collection_.find(offset);
            return it != collection_.end() ? it->second : nullptr;
        }

        // Lower case for template interop with std::map.
        uint32_t size() const { return collection_.size(); }

        std::map<uint32_t, T *> &Collection() { return collection_; }


        ~CollectionMap() {
            const char* symbol=typeid(T).name();
            int status;
            char *demangled = abi::__cxa_demangle(symbol, 0, 0, &status);
            int maps_size=size();



            //collection_.erase(collection_.end());xx

        }

    private:
        std::map<uint32_t, T *> collection_;

        void AddItem(T *object, uint32_t offset) {
            auto it = collection_.emplace(offset, object);
            CHECK(it.second) << "CollectionMap already has an object with offset " << offset << " "
                             << " and address " << it.first->second;
        }

        friend class Collections;
        DISALLOW_COPY_AND_ASSIGN(CollectionMap);
    };
}

#endif //UNPACKER_COLLECTIONMAP_H
