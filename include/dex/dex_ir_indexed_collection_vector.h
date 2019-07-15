/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:23 PM
* @ class describe
*/

#ifndef UNPACKER_INDEXEDCOLLECTIONVECTOR_H
#define UNPACKER_INDEXEDCOLLECTIONVECTOR_H


namespace dex_ir {
    template<class T>
    class IndexedCollectionVector : public CollectionVector<T> {
    public:
        using Vector = std::vector<std::unique_ptr<T>>;

        IndexedCollectionVector() = default;

    private:
        void AddIndexedItem(T *object, uint32_t index) {
            object->SetIndex(index);
            CollectionVector<T>::collection_.push_back(std::unique_ptr<T>(object));
        }

        friend class Collections;
        DISALLOW_COPY_AND_ASSIGN(IndexedCollectionVector);
    };

}

#endif //UNPACKER_INDEXEDCOLLECTIONVECTOR_H
