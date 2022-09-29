Fin.t seems to be too heavy with dependencies
for easy handling


Goals
* shallow (using Coq logic - no deep model) embedding into Coq (or Lean or Agda)
* keep it simple (probably no separation logic)


maybe look at 
* Andrew Apple C toolchain [VST](https://vst.cs.princeton.edu/)
* [Separation Logic](https://en.wikipedia.org/wiki/Separation_logic)
* [ocmal-to-coq](https://github.com/formal-land/coq-of-ocaml)
* [hs-to-coq](https://github.com/plclub/hs-to-coq) [Paper](https://arxiv.org/pdf/1711.09286.pdf) [Report](https://dl.acm.org/doi/pdf/10.1145/3236784)
* [ocaml-to-coq-axioms](https://dl.acm.org/doi/pdf/10.1145/1932681.1863590)
* [verified language overview](https://deepspec.org/main)
* [haskell to agda](https://dl.acm.org/doi/pdf/10.1145/1088348.1088355)
* [rust-to-lean](https://github.com/Kha/electrolysis)

quick takeaway from the papers:
* just do it
* normal shallow embeddings seems to work mostly

Problems:
* how to handle CPS => Prop return
* termination => disable checker, ind graphs, axiomatize, ...
* imperative features
* nested functions => inner lambda, everything mutual recursive
* Axiom semantics
* Fin type of Int/Idx
* modulo arithmetic
* highly dependent tuples (zipCalculus)



We look at:
``` 
f := λ a. (a+a)*2;

.cn f [a:Int 32, ret: Cn [Int 32]] = ret ((a+a)*2);
f [a:Int 32, ret: [Int 32] -> ⊥:*] -> ⊥:* = ret ((a+a)*2);
``` 


Idea: 
we interpret (⊥:*) as Prop =>
  final return of continuations is property

