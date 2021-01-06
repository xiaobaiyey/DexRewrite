
#include <stdlib.h>
#include <memory>
#include <libdex/dex/dex_file.h>
#include <libdex/header.h>
#include <libdex/decompilation.h>
#include <libdex/decompile_method.h>
#include <libbase/file.h>
#include <libbase/logging.h>
#include <assert.h>

/**
 * load a dex file
 * @return
 */
std::unique_ptr<libdex::DexFile> loadDexFile() {
    //read file
    std::string dex_data;
    auto read_success = base::ReadFileToString("classes.dex", &dex_data);
    assert(read_success);
    return std::unique_ptr<libdex::DexFile>(libdex::DexFile::getDexFile((uint8_t *) dex_data.data(), dex_data.size()));
}

/**
 * parse dex file
 * @param mDexFile
 * @return
 */
std::unique_ptr<dex_ir::Header> parseDexFile(std::unique_ptr<libdex::DexFile> mDexFile) {
    if (mDexFile->IsMagicValid() && mDexFile->IsVersionValid()) {
        return std::unique_ptr<dex_ir::Header>(dex_ir::DexIrBuilder(*mDexFile.get(), false));
    }
    return nullptr;
}


int main(int argc, char **argv) {

    auto mHeader = parseDexFile(loadDexFile());
    //Find all data references
    dex_ir::Decompilation decompilation(mHeader.get());
    if (decompilation.loadReferences()) {
        //find which method call System.exit()
        auto method_id = decompilation.getMethodIdBySignatrue("Ljava/lang/System;", "exit", "(I)V");
        if (method_id != nullptr) {

            //get all methods which call "exit"
            for (const auto &methodRef : method_id->GetMethodRefs()) {
                dex_ir::MethodItem *ref_method_item = mHeader->MethodItems()[methodRef];
                if (ref_method_item == nullptr) {
                    continue;
                }
                //get java style class name eg: java.lang.System
                auto java_class_name = ref_method_item->GetJavaClassName();
                LOG(DEBUG) << "java:" << java_class_name;
                //get vm style class name eg: Ljava/lang/System;
                auto raw_class_name = ref_method_item->GetRawClassName();
                LOG(DEBUG) << "raw:" << java_class_name;
                //get this method code
                auto code_item = ref_method_item->GetCodeItem();


            }
        }
    } else {
        LOG(ERROR) << "load dex references fail!!!!";
    }


    return 0;
}