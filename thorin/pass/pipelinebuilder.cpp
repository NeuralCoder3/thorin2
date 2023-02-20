#include "thorin/pass/pipelinebuilder.h"

#include "thorin/def.h"
#include "thorin/dialects.h"
#include "thorin/lattice.h"

#include "thorin/pass/fp/beta_red.h"
#include "thorin/pass/fp/copy_prop.h"
#include "thorin/pass/fp/eta_exp.h"
#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/fp/tail_rec_elim.h"
#include "thorin/pass/pass.h"
#include "thorin/pass/rw/partial_eval.h"
#include "thorin/pass/rw/ret_wrap.h"
#include "thorin/pass/rw/scalarize.h"
#include "thorin/phase/phase.h"

namespace thorin {

void PipelineBuilder::remember_pass_instance(Pass* p, const Def* def) {
    def->world().DLOG("associating {} with {}", def->gid(), p);
    pass_instances_[def] = p;
}
Pass* PipelineBuilder::get_pass_instance(const Def* def) { return pass_instances_[def]; }

void PipelineBuilder::begin_pass_phase() { man = std::make_unique<PassMan>(world_); }
void PipelineBuilder::end_pass_phase() {
    std::unique_ptr<thorin::PassMan>&& pass_man_ref = std::move(man);
    pipe->add<PassManPhase>(std::move(pass_man_ref));
    man = nullptr;
}

// void PipelineBuilder::extend_codegen_prep_phase(std::function<void(PassMan&)>&& extension) {
//     extend_opt_phase(Codegen_Prep_Phase, extension);
// }

// void PipelineBuilder::extend_opt_phase(int i, std::function<void(PassMan&)> extension, int priority) {
//     // adds extension to the i-th optimization phase
//     // if the ith phase does not exist, it is created
//     if (!phase_extensions_.contains(i)) { phase_extensions_[i] = std::vector<PrioPassBuilder>(); }
//     phase_extensions_[i].push_back({priority, extension});
// }

// void PipelineBuilder::add_opt(int i) {
//     extend_opt_phase(
//         i,
//         [](thorin::PassMan& man) {
//             man.add<PartialEval>();
//             auto br = man.add<BetaRed>();
//             auto er = man.add<EtaRed>();
//             auto ee = man.add<EtaExp>(er);
//             man.add<Scalerize>(ee);
//             man.add<TailRecElim>(er);
//             man.add<CopyProp>(br, ee);
//         },
//         Pass_Internal_Priority); // elevated priority
// }

// std::vector<int> PipelineBuilder::passes() {
//     std::vector<int> keys;
//     for (auto iter = phase_extensions_.begin(); iter != phase_extensions_.end(); iter++) {
//         keys.push_back(iter->first);
void PipelineBuilder::register_dialect(Dialect& dialect) { dialects_.push_back(&dialect); }
bool PipelineBuilder::is_registered_dialect(std::string name) {
    for (auto& dialect : dialects_) {
        if (dialect->name() == name) { return true; }
    }
    return false;
}

void PipelineBuilder::run_pipeline() { pipe->run(); }

} // namespace thorin
