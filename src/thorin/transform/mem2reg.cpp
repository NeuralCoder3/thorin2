#include "thorin/memop.h"
#include "thorin/world.h"
#include "thorin/analyses/scope.h"
#include "thorin/analyses/schedule.h"
#include "thorin/analyses/top_level_scopes.h"
#include "thorin/analyses/verify.h"

namespace thorin {

void mem2reg(const Scope& scope) {
    auto schedule = schedule_late(scope);
    DefMap<size_t> addresses;
    LambdaSet set;
    size_t cur_handle = 0;

    // unseal all lambdas ...
    for (auto lambda : scope.rpo()) {
        lambda->set_parent(lambda);
        lambda->unseal();
    }

    // ... except top-level lambdas
    scope.entry()->set_parent(0);
    scope.entry()->seal();

    for (Lambda* lambda : scope) {
        // Search for slots/loads/stores from top to bottom and use set_value/get_value to install parameters.
        for (auto primop : schedule[lambda]) {
            auto def = Def(primop);
            if (auto slot = def->isa<Slot>()) {
                // are all users loads and store?
                for (auto use : slot->uses()) {
                    if (!use->isa<Load>() && !use->isa<Store>()) {
                        addresses[slot] = size_t(-1);     // mark as "address taken"
                        goto next_primop;
                    }
                }
                addresses[slot] = cur_handle++;
            } else if (auto store = def->isa<Store>()) {
                if (auto slot = store->ptr()->isa<Slot>()) {
                    if (addresses[slot] != size_t(-1)) {  // if not "address taken"
                        lambda->set_value(addresses[slot], store->val());
                        store->replace(store->mem());
                    }
                }
            } else if (auto load = def->isa<Load>()) {
                if (auto slot = load->ptr()->isa<Slot>()) {
                    if (addresses[slot] != size_t(-1)) {  // if not "address taken"
                        auto type = slot->type()->as<Ptr>()->referenced_type();
                        load->replace(lambda->get_value(addresses[slot], type, slot->name.c_str()));
                    }
                }
            }
next_primop:;
        }

        // seal successors of last lambda if applicable
        for (auto succ : scope.succs(lambda)) {
            if (succ->parent() != 0) {
                if (!set.visit(succ)) {
                    assert(addresses.find(succ) == addresses.end());
                    addresses[succ] = succ->preds().size();
                }
                if (--addresses[succ] == 0)
                    succ->seal();
            }
        }
    }
}

void mem2reg(World& world) {
    for (auto scope : top_level_scopes(world))
        mem2reg(*scope);
    world.cleanup();
    debug_verify(world);
}

}
