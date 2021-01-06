//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_FIELD_ID_H
#define BASE_FIELD_ID_H

#include "indexed_item.h"
#include "dexir_util.h"
#include "type_id.h"
#include "string_id.h"

namespace dex_ir {
    class FieldId : public IndexedItem {
    public:
        FieldId(const TypeId *klass, const TypeId *type, const StringId *name);

        ~FieldId() override;

        static size_t ItemSize() { return kFieldIdItemSize; }

        const TypeId *Class() const;

        const TypeId *Type() const;

        const StringId *Name() const;

        //void Accept(AbstractDispatcher *dispatch) const { dispatch->Dispatch(this); }
        void AddMethodRefs(uint32_t id);

        std::set<uint32_t> GetMethodRefs() const;

    private:
        const TypeId *class_;
        const TypeId *type_;
        const StringId *name_;

        std::set<uint32_t> method_ref_idx;
        DISALLOW_COPY_AND_ASSIGN(FieldId);
    };

}


#endif //BASE_FIELD_ID_H
