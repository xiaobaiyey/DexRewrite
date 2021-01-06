//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_INDEXED_COLLECTION_VECTOR_H
#define BASE_INDEXED_COLLECTION_VECTOR_H

#include "collection_vector.h"

namespace dex_ir {
    template<class T>
    class IndexedCollectionVector : public CollectionVector<T> {
    public:
        using Vector = std::vector<std::unique_ptr<T>>;

        IndexedCollectionVector() = default;

        explicit IndexedCollectionVector(size_t size) : CollectionVector<T>(size) {}

        template<class... Args>
        T *CreateAndAddIndexedItem(uint32_t index, Args &&... args) {
            T *object = CollectionVector<T>::CreateAndAddItem(std::forward<Args>(args)...);
            object->SetIndex(index);
            return object;
        }

        T *operator[](size_t index) const {
            DCHECK_NE(CollectionVector<T>::collection_[index].get(), static_cast<T *>(nullptr));
            return CollectionVector<T>::collection_[index].get();
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(IndexedCollectionVector);
    };

}


#endif //BASE_INDEXED_COLLECTION_VECTOR_H
