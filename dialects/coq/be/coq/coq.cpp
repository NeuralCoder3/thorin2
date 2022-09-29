#include "dialects/coq/be/coq/coq.h"

#include <deque>
#include <fstream>
#include <iomanip>
#include <limits>
#include <ranges>

#include "thorin/analyses/cfg.h"
#include "thorin/be/emitter.h"
#include "thorin/util/print.h"
#include "thorin/util/sys.h"

using namespace std::string_literals;

namespace thorin::coq {

void emit(World& world, std::ostream& ostream) {
    // Emitter emitter(world, ostream);
    // emitter.run();
    ostream << "Hello World!" << std::endl;
}

int compile(World& world, std::string name) { throw std::runtime_error("compilation not supported"); }

int compile(World& world, std::string ll, std::string out) { throw std::runtime_error("compilation not supported"); }

int compile_and_run(World& world, std::string name, std::string args) {
    if (compile(world, name) == 0) return sys::run(name, args);
    throw std::runtime_error("compilation failed");
}

} // namespace thorin::coq