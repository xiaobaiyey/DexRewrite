//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_HIDDENAPI_CLASS_DATA_H
#define BASE_HIDDENAPI_CLASS_DATA_H


#include "libbase/safe_map.h"
#include "indexed_item.h"
#include "class_def.h"
#include <libbase/safe_map.h>

namespace dex_ir {
    class Header;
    using HiddenapiFlagsMap = base::SafeMap<const Item *, uint32_t>;

    class HiddenapiClassData : public IndexedItem {
    public:
        HiddenapiClassData(const ClassDef *class_def, std::unique_ptr<HiddenapiFlagsMap> flags)
                : class_def_(class_def), flags_(std::move(flags)) {}

        ~HiddenapiClassData() override {}

        const ClassDef *GetClassDef() const { return class_def_; }

        uint32_t GetFlags(const Item *field_or_method_item) const {
            return (flags_ == nullptr) ? 0u : flags_->Get(field_or_method_item);
        }

        static uint32_t GetFlags(Header *header, ClassDef *class_def, const Item *field_or_method_item);

        uint32_t ItemSize() const {
            uint32_t size = 0u;
            bool has_non_zero_entries = false;
            if (flags_ != nullptr) {
                for (const auto &entry : *flags_) {
                    size += base::UnsignedLeb128Size(entry.second);
                    has_non_zero_entries |= (entry.second != 0u);
                }
            }
            return has_non_zero_entries ? size : 0u;
        }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        const ClassDef *class_def_;
        std::unique_ptr<HiddenapiFlagsMap> flags_;
        DISALLOW_COPY_AND_ASSIGN(HiddenapiClassData);
    };
}

#endif //BASE_HIDDENAPI_CLASS_DATA_H
