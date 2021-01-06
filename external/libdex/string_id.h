//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_STRING_ID_H
#define BASE_STRING_ID_H

#include "indexed_item.h"
#include "string_data.h"
#include "dexir_util.h"

namespace dex_ir {
    class StringId : public IndexedItem {
    public:
        explicit StringId(StringData *string_data);

        ~StringId() override;

        static size_t ItemSize() ;

        const char *Data() const;

        StringData *DataItem() const;


        std::set<uint32_t> GetMethodRefs() const;

        void AddMethodRefs(uint32_t id);

    private:
        StringData *string_data_;
        std::set<uint32_t> method_ref_idx;
        DISALLOW_COPY_AND_ASSIGN(StringId);
    };
}

#endif //BASE_STRING_ID_H
