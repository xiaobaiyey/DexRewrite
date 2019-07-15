/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:05 AM
* @ class describe
*/



#include <dex/dex_ir_catchhandler.h>


namespace dex_ir {


    CatchHandler::CatchHandler(bool catch_all, uint16_t list_offset,
                               TypeAddrPairVector *handlers) : catch_all_(catch_all),
                                                               list_offset_(list_offset),
                                                               handlers_(handlers) {}
}
