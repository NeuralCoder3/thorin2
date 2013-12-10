#include "thorin/primop.h"

#include "thorin/literal.h"
#include "thorin/type.h"
#include "thorin/world.h"
#include "thorin/util/array.h"
#include "thorin/util/hash.h"

namespace thorin {

//------------------------------------------------------------------------------

void PrimOp::update(size_t i, Def with) { 
    unset_op(i); 
    set_op(i, with); 

    is_const_ = true;
    for (auto op : ops())
        if (op)
            is_const_ &= op->is_const();
}

VectorOp::VectorOp(size_t size, NodeKind kind, const Type* type, Def cond, const std::string& name)
    : PrimOp(size, kind, type, name)
{
    assert(cond->type()->is_u1());
    set_op(0, cond);
}

BinOp::BinOp(NodeKind kind, const Type* type, Def cond, Def lhs, Def rhs, const std::string& name)
    : VectorOp(3, kind, type, cond, name)
{
    assert(lhs->type() == rhs->type() && "types are not equal");
    set_op(1, lhs);
    set_op(2, rhs);
}

RelOp::RelOp(RelOpKind kind, Def cond, Def lhs, Def rhs, const std::string& name)
    : BinOp((NodeKind) kind, lhs->world().type_u1(lhs->type()->as<PrimType>()->length()), cond, lhs, rhs, name)
{}

Select::Select(Def cond, Def tval, Def fval, const std::string& name)
    : VectorOp(3, Node_Select, tval->type(), cond, name)
{
    set_op(1, tval);
    set_op(2, fval);
    assert(tval->type() == fval->type() && "types of both values must be equal");
}

//------------------------------------------------------------------------------

ArrayAgg::ArrayAgg(World& world, const Type* elem, ArrayRef<Def> args, const std::string& name)
    : Aggregate(Node_ArrayAgg, args, name)
{
    set_type(world.def_array(elem, args.size()));
#ifndef NDEBUG
    for (size_t i = 0, e = size(); i != e; ++i)
        assert(args[i]->type() == array_type()->elem_type());
#endif
}

Tuple::Tuple(World& world, ArrayRef<Def> args, const std::string& name)
    : Aggregate(Node_Tuple, args, name)
{
    Array<const Type*> elems(size());
    for (size_t i = 0, e = size(); i != e; ++i)
        elems[i] = args[i]->type();

    set_type(world.sigma(elems));
}

Vector::Vector(World& world, ArrayRef<Def> args, const std::string& name)
    : Aggregate(Node_Vector, args, name)
{
    if (const PrimType* primtype = args.front()->type()->isa<PrimType>()) {
        assert(primtype->length() == 1);
        set_type(world.type(primtype->primtype_kind(), args.size()));
    } else {
        const Ptr* ptr = args.front()->type()->as<Ptr>();
        assert(ptr->length() == 1);
        set_type(world.ptr(ptr->referenced_type(), args.size()));
    }
}

const ArrayType* ArrayAgg::array_type() const { return type()->as<ArrayType>(); }
const Sigma* Tuple::sigma() const { return type()->as<Sigma>(); }
const VectorType* Vector::vector_type() const { return type()->as<VectorType>(); }

Extract::Extract(Def agg, Def index, const std::string& name)
    : AggOp(2, Node_Extract, type(agg, index), agg, index, name)
{}

const Type* Extract::type(Def agg, Def index) {
    if (auto sigma = agg->type()->isa<Sigma>())
        return sigma->elem_via_lit(index);
    else if (auto array = agg->type()->isa<ArrayType>())
        return array->elem_type();
    assert(false && "TODO");
}

Insert::Insert(Def agg, Def index, Def value, const std::string& name)
    : AggOp(3, Node_Insert, type(agg), agg, index, name)
{
    set_op(2, value);
}

const Type* Insert::type(Def agg) { return agg->type(); }

//------------------------------------------------------------------------------

LEA::LEA(Def def, Def index, const std::string& name)
    : PrimOp(2, Node_LEA, nullptr, name)
{
    set_op(0, def);
    set_op(1, index);

    if (auto sigma = referenced_type()->isa<Sigma>())
        set_type(index->world().ptr(sigma->elem_via_lit(index)));
    else {
        auto array = referenced_type()->as<ArrayType>();
        set_type(index->world().ptr(array->elem_type()));;
    }
}

const Type* LEA::referenced_type() const { return ptr()->type()->as<Ptr>()->referenced_type(); }

//------------------------------------------------------------------------------

Addr::Addr(Def lambda, const std::string& name)
    : PrimOp(1, Node_Addr, lambda->world().ptr(lambda->type()), name)
{
    set_op(0, lambda);
}

const Pi* Addr::referenced_type() const { return lambda()->pi(); }

//------------------------------------------------------------------------------

const char* PrimOp::op_name() const {
    switch (kind()) {
#define THORIN_AIR_NODE(op, abbr) case Node_##op: return #abbr;
#include "thorin/tables/nodetable.h"
        default: THORIN_UNREACHABLE;
    }
}

const char* ArithOp::op_name() const {
    switch (kind()) {
#define THORIN_ARITHOP(op) case ArithOp_##op: return #op;
#include "thorin/tables/arithoptable.h"
        default: THORIN_UNREACHABLE;
    }
}

const char* RelOp::op_name() const {
    switch (kind()) {
#define THORIN_RELOP(op) case RelOp_##op: return #op;
#include "thorin/tables/reloptable.h"
        default: THORIN_UNREACHABLE;
    }
}

const char* ConvOp::op_name() const {
    switch (kind()) {
#define THORIN_CONVOP(op) case ConvOp_##op: return #op;
#include "thorin/tables/convoptable.h"
        default: THORIN_UNREACHABLE;
    }
}

//------------------------------------------------------------------------------

} // namespace thorin
