#ifndef ANYDSL2_BE_LLVM_H
#define ANYDSL2_BE_LLVM_H

#include "anydsl2/util/assert.h"

namespace llvm {
    class IRBuilderBase;
    class Type;
    class Value;
    class Module;
}

namespace anydsl2 {

class Def;
class Type;

class World;

class EmitHook {
public:
    virtual ~EmitHook() {}

    virtual void assign(llvm::IRBuilderBase* builder, llvm::Module* module) {}
    virtual llvm::Value* emit(const Def*) { ANYDSL2_UNREACHABLE; }
    virtual llvm::Type* map(const Type*) { ANYDSL2_UNREACHABLE; }
};

#ifdef LLVM_SUPPORT
void emit_llvm(World& world, EmitHook& hook);
#else
inline void emit_llvm(World& world, EmitHook& hook) {}
#endif
inline void emit_llvm(World& world) { EmitHook hook; emit_llvm(world, hook); }

} // namespace anydsl2

#endif
