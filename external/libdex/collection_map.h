//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_COLLECTION_MAP_H
#define BASE_COLLECTION_MAP_H

#include "collection_base.h"
#include "collection_vector.h"
#include <map>
namespace dex_ir{
    template<class T> class CollectionMap : public CollectionBase {
    public:
        CollectionMap() = default;
        ~CollectionMap() override { }

        template <class... Args>
        T* CreateAndAddItem(CollectionVector<T>& vector,
                            bool eagerly_assign_offsets,
                            uint32_t offset,
                            Args&&... args) {
            T* item = vector.CreateAndAddItem(std::forward<Args>(args)...);
            DCHECK(!GetExistingObject(offset));
            DCHECK(!item->OffsetAssigned());
            if (eagerly_assign_offsets) {
                item->SetOffset(offset);
            }
            AddItem(item, offset);
            return item;
        }

        // Returns the existing item if it is already inserted, null otherwise.
        T* GetExistingObject(uint32_t offset) {
            auto it = collection_.find(offset);
            return it != collection_.end() ? it->second : nullptr;
        }

        uint32_t Size() const override { return size(); }

        // Lower case for template interop with std::map.
        uint32_t size() const { return collection_.size(); }
        std::map<uint32_t, T*>& Collection() { return collection_; }

    private:
        std::map<uint32_t, T*> collection_;

        // CollectionMaps do not own the objects they contain, therefore AddItem is supported
        // rather than CreateAndAddItem.
        void AddItem(T* object, uint32_t offset) {
            auto it = collection_.emplace(offset, object);
            CHECK(it.second) << "CollectionMap already has an object with offset " << offset << " "
                             << " and address " << it.first->second;
        }

        DISALLOW_COPY_AND_ASSIGN(CollectionMap);
    };
}



#endif //BASE_COLLECTION_MAP_H
