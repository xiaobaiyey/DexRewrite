//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_COLLECTION_VECTOR_H
#define BASE_COLLECTION_VECTOR_H

#include "collection_base.h"
#include <memory>
#include <vector>
#include "dexir_util.h"
#include <algorithm>
#include <iostream>

namespace dex_ir {

    template<class T>
    class CollectionVector : public CollectionBase {
    public:
        using ElementType = std::unique_ptr<T>;

        CollectionVector() {}

        explicit CollectionVector(size_t size) {
            // Preallocate so that assignment does not invalidate pointers into the vector.
            collection_.reserve(size);
        }

        ~CollectionVector() override {}

        template<class... Args>
        T *CreateAndAddItem(Args &&... args) {
            T *object = new T(std::forward<Args>(args)...);
            collection_.push_back(std::unique_ptr<T>(object));
            return object;
        }

        uint32_t Size() const override { return collection_.size(); }

        Iterator<ElementType> begin() const { return Iterator<ElementType>(collection_, 0U, Size()); }

        Iterator<ElementType> end() const { return Iterator<ElementType>(collection_, Size(), Size()); }

        const ElementType &operator[](size_t index) const {
            DCHECK_LT(index, Size());
            return collection_[index];
        }

        ElementType &operator[](size_t index) {
            DCHECK_LT(index, Size());
            return collection_[index];
        }


        // Sort the vector by copying pointers over.
        template<typename MapType>
        void SortByMapOrder(const MapType &map) {
            auto it = map.begin();
            CHECK_EQ(map.size(), Size());
            for (size_t i = 0; i < Size(); ++i) {
                // There are times when the array will temporarily contain the same pointer twice, doing the
                // release here sure there is no double free errors.
                collection_[i].release();
                collection_[i].reset(it->second);
                ++it;
            }
        }

    protected:
        std::vector<ElementType> collection_;

    private:
        DISALLOW_COPY_AND_ASSIGN(CollectionVector);
    };

}
#endif //BASE_COLLECTION_VECTOR_H
