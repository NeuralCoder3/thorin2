#include "dialects/ad_ext/ad_ext.h"

#include <thorin/config.h>
#include <thorin/dialects.h>
#include <thorin/pass/pass.h>

using namespace thorin;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() { return {"ad_ext", nullptr, nullptr, nullptr}; }
