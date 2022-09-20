#include "thorin/pass/pipelinebuilder.h"

#include <compare>

#include <vector>

#include "thorin/def.h"
#include "thorin/lattice.h"

#include "thorin/pass/fp/beta_red.h"
#include "thorin/pass/fp/eta_exp.h"
#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/fp/tail_rec_elim.h"
#include "thorin/pass/rw/partial_eval.h"
#include "thorin/pass/rw/ret_wrap.h"
#include "thorin/pass/rw/scalarize.h"

#include "dialects/mem/passes/fp/copy_prop.h"
#include "dialects/mem/passes/fp/ssa_constr.h"
#include "dialects/mem/passes/rw/alloc2malloc.h"
#include "dialects/mem/passes/rw/remem_elim.h"

namespace thorin {

void PipelineBuilder::extend_opt_phase(std::function<void(PassMan&)> extension) {
    extend_opt_phase(100, std::move(extension));
}

void PipelineBuilder::extend_codegen_prep_phase(std::function<void(PassMan&)> extension) {
    extend_opt_phase(300, std::move(extension));
}

void PipelineBuilder::extend_opt_phase(int i, std::function<void(PassMan&)> extension, int priority) {
    // adds extension to the i-th optimization phase
    // if the ith phase does not exist, it is created
    if (!phase_extensions_.contains(i)) {
        // phase_extensions_.emplace(i, std::vector<>());
        phase_extensions_[i] = std::vector<PrioPassBuilder>();
    }
    phase_extensions_[i].push_back({priority, extension});
}

void PipelineBuilder::add_opt(int i) {
    extend_opt_phase(
        i,
        [](thorin::PassMan& man) {
            man.add<PartialEval>();
            man.add<BetaRed>();
            auto er = man.add<EtaRed>();
            auto ee = man.add<EtaExp>(er);
            man.add<Scalerize>(ee);
            man.add<TailRecElim>(er);
        },
        50); // elevated priority
}

std::vector<int> PipelineBuilder::passes() {
    std::vector<int> keys;
    for (auto iter = phase_extensions_.begin(); iter != phase_extensions_.end(); iter++) {
        keys.push_back(iter->first);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

std::unique_ptr<PassMan> PipelineBuilder::opt_phase(int i, World& world) {
    auto man = std::make_unique<PassMan>(world);

    std::sort(phase_extensions_[i].begin(), phase_extensions_[i].end(), passCmp());

    // printf("====\n");
    for (const auto& ext : phase_extensions_[i]) {
        // printf("add pass with prio %d\n", ext.first);
        ext.second(*man);
    }
    // printf("====\n");
    // assert(0);

    return man;
}

} // namespace thorin
