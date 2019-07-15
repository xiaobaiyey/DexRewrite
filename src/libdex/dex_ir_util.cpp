/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019-04-18 21:57
* @ class describe
*/

#include "dex/dex_ir_util.h"
#include <dex/dex_ir_protoid.h>
#include <dex/dex_ir_typelist.h>
#include <dex/dex_ir_stringid.h>
#include <dex/dex_ir_stringdata.h>

namespace dex_ir{
    std::string DexIRUtil::GetSignatureForProtoId(const ProtoId *proto) {
        if (proto == nullptr) {
            return "<no signature>";
        }

        std::string result("(");
        const dex_ir::TypeList* type_list = proto->Parameters();
        if (type_list != nullptr) {
            for (const dex_ir::TypeId* type_id : *type_list->GetTypeList()) {
                result += type_id->GetStringId()->Data();
            }
        }
        result += ")";
        result += proto->ReturnType()->GetStringId()->Data();
        return result;
    }


     std::string DexIRUtil::DescriptorToDot(const char* descriptor) {
        size_t length = strlen(descriptor);
        if (length > 1) {
            if (descriptor[0] == 'L' && descriptor[length - 1] == ';') {
                // Descriptors have the leading 'L' and trailing ';' stripped.
                std::string result(descriptor + 1, length - 2);
                std::replace(result.begin(), result.end(), '/', '.');
                return result;
            } else {
                // For arrays the 'L' and ';' remain intact.
                std::string result(descriptor);
                std::replace(result.begin(), result.end(), '/', '.');
                return result;
            }
        }
        // Do nothing for non-class/array descriptors.
        return descriptor;
    }

    std::string DexIRUtil::DescriptorToDotWrapper(const char *descriptor) {
        std::string result = DescriptorToDot(descriptor);
        size_t found = result.find('$');
        while (found != std::string::npos) {
            result[found] = '.';
            found = result.find('$', found);
        }
        return result;
    }
}
