//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_TYPE_ID_H
#define BASE_TYPE_ID_H


#include "indexed_item.h"
#include "dexir_util.h"
#include "string_id.h"

namespace dex_ir {
    class TypeId : public IndexedItem {
    public:
        explicit TypeId(StringId *string_id);

        ~TypeId() override;

        static size_t ItemSize() { return kTypeIdItemSize; }

        StringId *GetStringId() const;

        //void Accept(AbstractDispatcher *dispatch) const { dispatch->Dispatch(this); }

        void AddMethodRef(uint32_t id);

        std::set<uint32_t> GetMethodRefs() const;

    private:
        StringId *string_id_;
        //value=method index id
        std::set<uint32_t> method_ref_idx;
        DISALLOW_COPY_AND_ASSIGN(TypeId);
    };

    using TypeIdVector = std::vector<const TypeId *>;
}

#endif //BASE_TYPE_ID_H
