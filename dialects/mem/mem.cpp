#include "dialects/mem/mem.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/axiom.h"
#include "thorin/dialects.h"

#include "thorin/pass/fp/beta_red.h"
#include "thorin/pass/fp/eta_exp.h"
#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/fp/tail_rec_elim.h"
#include "thorin/pass/rw/partial_eval.h"
#include "thorin/pass/rw/ret_wrap.h"
#include "thorin/pass/rw/scalarize.h"

#include "dialects/mem/autogen.h"
#include "dialects/mem/passes/fp/copy_prop.h"
#include "dialects/mem/passes/fp/ssa_constr.h"
#include "dialects/mem/passes/rw/alloc2malloc.h"
#include "dialects/mem/passes/rw/remem_elim.h"
#include "dialects/mem/passes/rw/reshape.h"
#include "dialects/mem/phases/rw/add_mem.h"

using namespace thorin;

extern "C" THORIN_EXPORT DialectInfo thorin_get_dialect_info() {
    return {"mem",
            [](PipelineBuilder& builder) {
                builder.extend_opt_phase([](PassMan& man) {
                    auto br = man.add<BetaRed>();
                    auto er = man.add<EtaRed>();
                    auto ee = man.add<EtaExp>(er);
                    man.add<mem::SSAConstr>(ee);
                    man.add<mem::CopyProp>(br, ee);
                });
                builder.extend_codegen_prep_phase([](PassMan& man) {
                    man.add<mem::RememElim>();
                    man.add<mem::Alloc2Malloc>();
                });
                // builder.extend_opt_phase(104, [](PassMan& man) { man.add<mem::Reshape>(mem::Reshape::Arg); });
                // builder.append_phase(130, [](Pipeline& pipeline) { pipeline.add<mem::AddMem>(); });

                // after AD, before closure conv
                // builder.extend_opt_phase(139, [](PassMan& man) { man.add<mem::Reshape>(mem::Reshape::Flat); });
            },
            [](Passes& passes) {
                register_pass_with_arg<mem::ssa_pass, mem::SSAConstr, EtaExp>(passes);
                register_pass<mem::remem_elim_pass, mem::RememElim>(passes);
                register_pass<mem::Alloc2Malloc, mem::Alloc2Malloc>(passes);

                // passes[flags_t(Axiom::Base<mem::copy_prop_no_arg_pass>)] = [&](World&, PipelineBuilder& builder,
                //                                                                const Def* app) {
                //     auto bb_only = app->as<App>()->arg()->as<Lit>()->get<u64>();
                //     builder.add_pass<mem::CopyProp>(app, nullptr, nullptr, bb_only);
                // };

                // TODO: generalize register_pass_with_arg
                passes[flags_t(Axiom::Base<mem::copy_prop_pass>)] = [&](World& world, PipelineBuilder& builder,
                                                                        const Def* app) {
                    auto [br, ee, bb] = app->as<App>()->args<3>();
                    // TODO: let get_pass do the casts
                    auto br_pass = (BetaRed*)builder.get_pass_instance(br);
                    auto ee_pass = (EtaExp*)builder.get_pass_instance(ee);
                    auto bb_only = bb->as<Lit>()->get<u64>();
                    world.DLOG("registering copy_prop with br = {}, ee = {}, bb_only = {}", br, ee, bb_only);
                    builder.add_pass<mem::CopyProp>(app, br_pass, ee_pass, bb_only);
                };
                passes[flags_t(Axiom::Base<mem::reshape_pass>)] = [&](World&, PipelineBuilder& builder,
                                                                      const Def* app) {
                    auto mode_ax = app->as<App>()->arg()->as<Axiom>();
                    auto mode    = mode_ax->flags() == flags_t(Axiom::Base<mem::reshape_arg>) ? mem::Reshape::Arg
                                                                                              : mem::Reshape::Flat;
                    builder.add_pass<mem::Reshape>(app, mode);
                };
                register_pass<mem::add_mem_pass, mem::AddMemWrapper>(passes);
            },
            nullptr, [](Normalizers& normalizers) { mem::register_normalizers(normalizers); }};
}
