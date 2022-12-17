#include "thorin/analyses/deptree.h"

#include "dialects/autodiff/autogen.h"
#include "dialects/autodiff/passes/autodiff_eval.h"
#include "dialects/autodiff/passes/def_inliner.h"
#include "dialects/autodiff/utils/builder.h"
#include "dialects/autodiff/utils/helper.h"
#include "dialects/math/math.h"
#include "dialects/mem/autogen.h"
#include "dialects/mem/mem.h"

namespace thorin::autodiff {

void AutoDiffEval::assign_gradients(Lam* diffee, Lam* diff) {
    auto num        = diff->num_vars();
    size_t diffee_i = 0;
    size_t diff_i   = 0;
    for (; diff_i < num;) {
        auto def = diff->var(diff_i++);
        if (match<mem::Ptr>(def->type())) {
            auto values   = diffee->var(diffee_i);
            auto gradient = diff->var(diff_i++);

            gradient_pointers[values] = gradient;
        }
        diffee_i++;
    }
}

const Def* AutoDiffEval::input_mapping(Lam* forward) {
    auto& w  = world();
    auto num = forward->num_vars();
    size_t i = 0;
    DefVec var_mapping;
    for (; i < num;) {
        auto values = forward->var(i++);
        var_mapping.push_back(values);
        if (match<mem::Ptr>(values->type())) { i++; }
    }

    return w.tuple(var_mapping);
}

Lam* AutoDiffEval::free_memory() {
    auto free_memory = create_block("free_memory");
    for( auto ptr : need_free ){
        op_free(ptr);
    }
    ret(free_memory);
    return free_memory;
}

Lam* AutoDiffEval::init_caches(Lam* next) {
    auto& w        = world();
    Lam* current   = build(w).mem_ty().lam("init_caches");
    InitFrame last = {.lam = current, .mem = mem::mem_var(current)};
    for (; !init_frames.empty();) {
        auto next = init_frames.top();
        init_frames.pop();
        last.lam->set_body(w.app(next.lam, last.mem));
        last = next;
    }

    last.lam->set_body(w.app(next, last.mem));
    return current;
}

bool check_vars(const Def* def, DefSet& vars) {
    if (def->isa<Var>()) {
        return vars.contains(def);
    } else if (!def->isa_nom<Lam>()) {
        for (auto op : def->ops()) {
            if (!check_vars(op, vars)) { return false; }
        }
    }

    return true;
}

void validate(Lam* lam, DefSet& vars) {
    DefSet new_vars = vars;
    new_vars.insert(lam->var());
    auto body = lam->body()->as<App>();
    assert(check_vars(body, new_vars));
    for_each_lam(body, [&](Lam* next) { validate(next, new_vars); });
}

void validate(Lam* lam) {
    DefSet vars;
    return validate(lam, vars);
}

void AutoDiffEval::init_loop_frame() {
    root    = std::make_shared<LoopFrame>(*this);
    auto& w = world();
    LoopData data;

    auto index_ty    = w.type_int(64);
    root->size       = one(index_ty);
    root->local_size = one(index_ty);
    data.index       = zero(index_ty);
    data.cache_index = zero(index_ty);
    root->forward    = data;
    root->backward   = data;
    current_loop     = root;
}

const Def* call_arg(const App* app) {
    for (auto use : app->uses()) {
        if (auto app = use->isa<App>()) {
            return app->arg();
            /*for( auto proj : app->args() ){
                if( match<mem::Ptr>(proj->type()) && is_null(proj) ){
                    proj->dump();
                }
            }*/
        }
    }

    return nullptr;
}

bool is_null(const Def* def) {
    if (auto lit = isa_lit(def)) {
        return lit == 0;
    } else if (auto bitcast = match<core::bitcast>(def)) {
        return is_null(bitcast->arg());
    }

    return false;
}

void AutoDiffEval::init_grad_requirements(const App* ad) {
    auto& gradient = factory->gradient();
    auto lam       = factory->lam();
    auto arg       = call_arg(ad);

    size_t i = 0;
    for (auto var : lam->vars()) {
        if (match<mem::M>(var->type())) {
            i++;
            continue;
        }

        if (arg && match<mem::Ptr>(var->type())) {
            i++;
            if (is_null(arg->proj(i))) {
                i++;
                continue;
            }
        }

        // gradients of input arguments are required
        gradient.require(var);

        // we have the gradients of all input pointers
        if (match<mem::Ptr>(var->type())) { gradient.has(var); }

        i++;
    }

    auto& cfa     = factory->cfa();
    auto ret_node = cfa.node(lam->ret_var());
    auto ret_wrap = ret_node->pred();
    auto ret_app  = ret_wrap->def()->as_nom<Lam>()->body()->as<App>();
    auto ret_arg  = ret_app->arg();

    for (auto proj : ret_arg->projs()) {
        if (match<mem::M>(proj->type())) continue;
        gradient.has(proj);
    }
}

const Def* AutoDiffEval::derive_(const Def* def) {
    auto& w = world();

    auto ad_app = force<autodiff::ad>(def);

    auto diffee = ad_app->arg()->isa_nom<Lam>();
    assert(diffee);

    DefInliner inliner(diffee);
    inliner.forbid<mem::Ptr>();
    inliner.filter([](const Def* def) { return !is_idx(def->type()); });
    diffee  = inliner.build();
    factory = std::make_unique<AnalysisFactory>(diffee);

    init_loop_frame();
    build_branch_table(diffee);
    init_grad_requirements(ad_app);

    auto diff_ty = autodiff_type_fun_pi(diffee->type());

    auto diff_lam = create_lam(diff_ty, "diff_lam_" + diffee->name());

    auto diff_ret_type  = diff_ty->dom(diff_ty->num_doms() - 1)->as<Pi>();
    auto inv_lam_ty     = diff_ret_type->dom(diff_ret_type->num_doms() - 1)->as<Pi>();
    auto backward_begin = create_lam(inv_lam_ty, "backward_begin_" + diffee->name());

    auto lam_return_ty = diffee->type()->ret_pi();
    auto forward_end   = create_lam(lam_return_ty, "forward_end_" + diffee->name());

    assign_gradients(diffee, diff_lam);
    auto mapping             = mask_last(input_mapping(diff_lam), forward_end);
    augmented[diffee->var()] = mapping;
    add_inverted(diffee->var(), mapping);

    current_state      = State::Augment;
    auto forward_begin = push_lam(w.cn(mem::type_mem(w)), "forward_begin_" + diffee->name());
    forward_begin->set_body(augment(diffee->body()));

    current_state   = State::Invert;
    auto inv_diffee = invert_lam(diffee);
    auto test       = current_mem;

    forward_end->set_body(w.app(diff_lam->ret_var(), merge_flat(forward_end->var(), backward_begin)));
    auto backward_end = create_lam(inv_diffee->ret_pi(), "backward_end_" + diffee->name());

    DefVec gradient_results;
    for (auto var : backward_begin->args()) { gradient_results.push_back(var); }
    gradient_results.push_back(backward_end);

    auto free_lam = free_memory();
    auto free_ret = create_lam(free_lam->ret_pi(), "ret_" + free_lam->name());

    backward_begin->set_body(w.app(inv_diffee, gradient_results));

    backward_end->set_body(w.app(free_lam, {mem::mem_var(backward_end), free_ret}));
    free_ret->set_body(w.app(backward_begin->ret_var(), mem::replace_mem(mem::mem_var(free_ret), backward_end->var())));

    diff_lam->set_body(w.app(init_caches(forward_begin), mem::mem_var(diff_lam)));

    assert(!current_mem);

    validate(diff_lam);
    return diff_lam;
}

} // namespace thorin::autodiff