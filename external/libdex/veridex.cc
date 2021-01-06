/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "veridex.h"

#include "libbase/file.h"

#include "dex/dex_file.h"

#include "hidden_api.h"
#include "hidden_api_finder.h"
#include "precise_hidden_api_finder.h"
#include "resolver.h"

#include <cstdlib>
#include <sstream>
#include "dex/hiddenapi_flags.h"
#include "dex/standard_dex_file.h"

namespace dex_ir {

    static VeriClass z_(libdex::Primitive::Type::kPrimBoolean, 0, nullptr);
    static VeriClass b_(libdex::Primitive::Type::kPrimByte, 0, nullptr);
    static VeriClass c_(libdex::Primitive::Type::kPrimChar, 0, nullptr);
    static VeriClass s_(libdex::Primitive::Type::kPrimShort, 0, nullptr);
    static VeriClass i_(libdex::Primitive::Type::kPrimInt, 0, nullptr);
    static VeriClass f_(libdex::Primitive::Type::kPrimFloat, 0, nullptr);
    static VeriClass d_(libdex::Primitive::Type::kPrimDouble, 0, nullptr);
    static VeriClass j_(libdex::Primitive::Type::kPrimLong, 0, nullptr);
    static VeriClass v_(libdex::Primitive::Type::kPrimVoid, 0, nullptr);

    VeriClass *VeriClass::boolean_ = &z_;
    VeriClass *VeriClass::byte_ = &b_;
    VeriClass *VeriClass::char_ = &c_;
    VeriClass *VeriClass::short_ = &s_;
    VeriClass *VeriClass::integer_ = &i_;
    VeriClass *VeriClass::float_ = &f_;
    VeriClass *VeriClass::double_ = &d_;
    VeriClass *VeriClass::long_ = &j_;
    VeriClass *VeriClass::void_ = &v_;

// Will be set after boot classpath has been resolved.
    VeriClass *VeriClass::object_ = nullptr;
    VeriClass *VeriClass::class_ = nullptr;
    VeriClass *VeriClass::class_loader_ = nullptr;
    VeriClass *VeriClass::string_ = nullptr;
    VeriClass *VeriClass::throwable_ = nullptr;
    VeriMethod VeriClass::forName_ = nullptr;
    VeriMethod VeriClass::getField_ = nullptr;
    VeriMethod VeriClass::getDeclaredField_ = nullptr;
    VeriMethod VeriClass::getMethod_ = nullptr;
    VeriMethod VeriClass::getDeclaredMethod_ = nullptr;
    VeriMethod VeriClass::getClass_ = nullptr;
    VeriMethod VeriClass::loadClass_ = nullptr;
    VeriField VeriClass::sdkInt_ = nullptr;

    static const char *kDexFileOption = "--dex-file=";
    static const char *kStubsOption = "--core-stubs=";
    static const char *kFlagsOption = "--api-flags=";
    static const char *kImprecise = "--imprecise";
    static const char *kTargetSdkVersion = "--target-sdk-version=";
    static const char *kOnlyReportSdkUses = "--only-report-sdk-uses";


    static const char *Substr(const char *str, int index) {
        return str + index;
    }

    static bool StartsWith(const char *str, const char *val) {
        return strlen(str) >= strlen(val) && memcmp(str, val, strlen(val)) == 0;
    }

    static void ParseArgs(VeridexOptions *options, int argc, char **argv) {
        // Skip over the command name.
        argv++;
        argc--;

        for (int i = 0; i < argc; ++i) {
            if (StartsWith(argv[i], kDexFileOption)) {
                options->dex_file = Substr(argv[i], strlen(kDexFileOption));
            } else if (StartsWith(argv[i], kStubsOption)) {
                options->core_stubs = Substr(argv[i], strlen(kStubsOption));
            } else if (StartsWith(argv[i], kFlagsOption)) {
                options->flags_file = Substr(argv[i], strlen(kFlagsOption));
            } else if (strcmp(argv[i], kImprecise) == 0) {
                options->precise = false;
            } else if (StartsWith(argv[i], kTargetSdkVersion)) {
                options->target_sdk_version = atoi(Substr(argv[i], strlen(kTargetSdkVersion)));
            } else if (strcmp(argv[i], kOnlyReportSdkUses) == 0) {
                options->only_report_sdk_uses = true;
            }
        }
    }

    static std::vector<std::string> Split(const std::string &str, char sep) {
        std::vector<std::string> tokens;
        std::string tmp;
        std::istringstream iss(str);
        while (std::getline(iss, tmp, sep)) {
            tokens.push_back(tmp);
        }
        return tokens;
    }

    class Veridex {
    public:
        static int Run(VeridexOptions *options, std::map<std::string, std::vector<std::string>> &greylist,
                       std::map<std::string, std::vector<std::string>> &blacklist,
                       std::map<std::string, std::vector<std::string>> &greylist_max_o,
                       std::map<std::string, std::vector<std::string>> &greylist_max_p) {


            if (options->dex_files_path.size() <= 0) {
                LOG(ERROR) << "Required argument '" << kDexFileOption << "' not provided.";
                return 1;
            }

            gTargetSdkVersion = options->target_sdk_version;

            std::vector<std::string> boot_content;
            std::vector<std::string> app_content;
            std::vector<std::unique_ptr<const libdex::DexFile>> boot_dex_files;
            std::vector<std::unique_ptr<const libdex::DexFile>> app_dex_files;
            std::string error_msg;

            // Read the boot classpath.
            std::vector<std::string> boot_classpath = Split(options->core_stubs, ':');
            boot_content.resize(boot_classpath.size());
            uint32_t i = 0;
            for (const std::string &str : boot_classpath) {
                if (!Load(str, boot_content[i++], &boot_dex_files, &error_msg)) {
                    LOG(ERROR) << error_msg;
                    return 1;
                }
            }
            // Read the apps dex files.
            std::vector<std::string> app_files = options->dex_files_path;
            app_content.resize(app_files.size());
            i = 0;
            for (const std::string &str : app_files) {
                if (!Load(str, app_content[i++], &app_dex_files, &error_msg)) {
                    LOG(ERROR) << error_msg;
                    return 1;
                }
            }

            // Resolve classes/methods/fields defined in each dex file.

            // Cache of types we've seen, for quick class name lookups.
            TypeMap type_map;
            // Add internally defined primitives.
            type_map["Z"] = VeriClass::boolean_;
            type_map["B"] = VeriClass::byte_;
            type_map["S"] = VeriClass::short_;
            type_map["C"] = VeriClass::char_;
            type_map["I"] = VeriClass::integer_;
            type_map["F"] = VeriClass::float_;
            type_map["D"] = VeriClass::double_;
            type_map["J"] = VeriClass::long_;
            type_map["V"] = VeriClass::void_;

            // Cache of resolvers, to easily query address in memory to VeridexResolver.
            DexResolverMap resolver_map;

            std::vector<std::unique_ptr<VeridexResolver>> boot_resolvers;
            Resolve(boot_dex_files, resolver_map, type_map, &boot_resolvers);

            // Now that boot classpath has been resolved, fill classes and reflection
            // methods.
            VeriClass::object_ = type_map["Ljava/lang/Object;"];
            VeriClass::class_ = type_map["Ljava/lang/Class;"];
            VeriClass::class_loader_ = type_map["Ljava/lang/ClassLoader;"];
            VeriClass::string_ = type_map["Ljava/lang/String;"];
            VeriClass::throwable_ = type_map["Ljava/lang/Throwable;"];
            VeriClass::forName_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_, "forName", "(Ljava/lang/String;)Ljava/lang/Class;");
            VeriClass::getField_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_, "getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
            VeriClass::getDeclaredField_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
            VeriClass::getMethod_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_,
                    "getMethod",
                    "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
            VeriClass::getDeclaredMethod_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_,
                    "getDeclaredMethod",
                    "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
            VeriClass::getClass_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::object_, "getClass", "()Ljava/lang/Class;");
            VeriClass::loadClass_ = boot_resolvers[0]->LookupDeclaredMethodIn(
                    *VeriClass::class_loader_, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

            VeriClass *version = type_map["Landroid/os/Build$VERSION;"];
            if (version != nullptr) {
                VeriClass::sdkInt_ = boot_resolvers[0]->LookupFieldIn(*version, "SDK_INT", "I");
            }

            std::vector<std::unique_ptr<VeridexResolver>> app_resolvers;
            Resolve(app_dex_files, resolver_map, type_map, &app_resolvers);

            // Find and log uses of hidden APIs.
            HiddenApi hidden_api(options->flags_file, options->only_report_sdk_uses);
            HiddenApiStats stats;

            HiddenApiFinder api_finder(hidden_api);
            api_finder.Run(app_resolvers);
            api_finder.Dump(std::cout, &stats, !options->precise, greylist, blacklist, greylist_max_o, greylist_max_p);

            if (options->precise) {
                PreciseHiddenApiFinder precise_api_finder(hidden_api);
                precise_api_finder.Run(app_resolvers);
                precise_api_finder.Dump(std::cout, &stats, greylist, blacklist, greylist_max_o, greylist_max_p);
            }
            DumpSummaryStats(std::cout, stats, options);

            if (options->precise) {
                std::cout << "To run an analysis that can give more reflection accesses, " << std::endl
                          << "but could include false positives, pass the --imprecise flag. " << std::endl;
            }

            return 0;
        }

    private:
        static void DumpSummaryStats(std::ostream &os,
                                     const HiddenApiStats &stats,
                                     const VeridexOptions *options) {
            static const char *kPrefix = "       ";
            if (options->only_report_sdk_uses) {
                os << stats.api_counts[libdex::hiddenapi::ApiList::Whitelist().GetIntValue()]
                   << " SDK API uses." << std::endl;
            } else {
                os << stats.count << " hidden API(s) used: "
                   << stats.linking_count << " linked against, "
                   << stats.reflection_count << " through reflection" << std::endl;
                for (size_t i = 0; i < libdex::hiddenapi::ApiList::kValueCount; ++i) {
                    libdex::hiddenapi::ApiList api_list = libdex::hiddenapi::ApiList(i);
                    if (api_list != libdex::hiddenapi::ApiList::Whitelist()) {
                        os << kPrefix << stats.api_counts[i] << " in " << api_list << std::endl;
                    }
                }
            }
        }

        static bool Load(const std::string &filename,
                         std::string &content,
                         std::vector<std::unique_ptr<const libdex::DexFile>> *dex_files,
                         std::string *error_msg) {
            if (filename.empty()) {
                *error_msg = "Missing file name";
                return false;
            }

            // TODO: once added, use an api to android::base to read a std::vector<uint8_t>.
            if (!base::ReadFileToString(filename.c_str(), &content)) {
                *error_msg = "ReadFileToString failed for " + filename;
                return false;
            }
            static constexpr bool kVerifyChecksum = false;
            static constexpr bool kRunDexFileVerifier = true;

            const libdex::DexFile::Header *dex_header = reinterpret_cast<const libdex::DexFile::Header *>(content.data());
            std::unique_ptr<const libdex::DexFile> dex_file;
            dex_file.reset(libdex::DexFile::getDexFile((const uint8_t *) content.data(), content.size()));
            dex_files->push_back(std::move(dex_file));
            return true;
        }

        static void Resolve(const std::vector<std::unique_ptr<const libdex::DexFile>> &dex_files,
                            DexResolverMap &resolver_map,
                            TypeMap &type_map,
                            std::vector<std::unique_ptr<VeridexResolver>> *resolvers) {
            for (const std::unique_ptr<const libdex::DexFile> &dex_file : dex_files) {
                VeridexResolver *resolver =
                        new VeridexResolver(*dex_file.get(), resolver_map, type_map);
                resolvers->emplace_back(resolver);
                resolver_map[reinterpret_cast<uintptr_t>(dex_file->Begin())] = resolver;
            }

            for (const std::unique_ptr<VeridexResolver> &resolver : *resolvers) {
                resolver->Run();
            }
        }
    };

    int CheckHiddenAPI(VeridexOptions *veridexOptions, std::map<std::string, std::vector<std::string>> &greylist,
                       std::map<std::string, std::vector<std::string>> &blacklist,
                       std::map<std::string, std::vector<std::string>> &greylist_max_o,
                       std::map<std::string, std::vector<std::string>> &greylist_max_p) {
        return dex_ir::Veridex::Run(veridexOptions, greylist, blacklist, greylist_max_o, greylist_max_p);
    }

}  // namespace dex_ir

/*
int main(int argc, char** argv) {
  return art::Veridex::Run(argc, argv);
}
*/
