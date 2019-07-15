/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:12 AM
* @ class describe
*/


#include <dex/dex_ir_code_item.h>
#include <dex/dex_instruction_iterator.h>

namespace dex_ir {

    CodeItem::CodeItem(uint16_t registers_size, uint16_t ins_size, uint16_t outs_size,
                       DebugInfoItem *debug_info, uint32_t insns_size, uint16_t *insns,
                       TryItemVector *tries, CatchHandlerVector *handlers) : registers_size_(registers_size),
                                                                             ins_size_(ins_size),
                                                                             outs_size_(outs_size),
                                                                             debug_info_(debug_info),
                                                                             insns_size_(insns_size),
                                                                             insns_(insns),
                                                                             tries_(tries),
                                                                             handlers_(handlers) {


    }

    void CodeItem::SetMethodItem(MethodItem *&ethodItem) {
        methodItem_ = ethodItem;
    }

    MethodItem *CodeItem::GetMethodItem() const {
        return methodItem_;
    }

    art::IterationRange<art::DexInstructionIterator> CodeItem::Instructions() const {
        return MakeIterationRange(art::DexInstructionIterator(Insns(), 0u),
                                  art::DexInstructionIterator(Insns(), InsnsSize()));
    }


}

