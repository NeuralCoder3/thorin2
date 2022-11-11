(* Load defs. *)

(* deep embedding *)


(* or use referencing (var) for lam *)
  Unset Positivity Checking.

(* types are also expressions *)
Inductive expression :=
(* | Var : nat -> expression *)
(*      name *)
| Nat : nat -> expression
| App : expression -> expression -> expression
(*      callee        args *)
| Arr : expression -> expression -> expression
(*      shape         body *)
| Extract: expression -> expression -> expression
(*         tuple         index *)
| Pack : expression -> expression -> expression
(*       shape         body *)
| Insert : expression -> expression -> expression -> expression
(*       tuple         index            value *)
| Idx : expression -> expression
(*       width *)
| Lit : expression -> nat -> expression
(*      type          value *)
| Pi : expression -> expression -> expression
(*     dom         codom *)
| TType : nat -> expression
(*        level *)
| Sigma : list expression -> expression
(*        type fields *)
| Tuple : list expression -> expression
(*        expression fields *)
(* TODO: needs type? *)

(* nom *)
| Lam : expression -> expression-> (expression -> expression) -> expression
(*      type          filter       var -> body *)
(* | nom_Sigma :  *)
(* | nom_Pack :  *)

(* 
Axiom
*)

(*
Pick, Proxy, Singelton, Test, Univ, Vel
*)
.

(*
https://github.com/AnyDSL/thorin2/blob/master/thorin/def.cpp#L73
*)


(* type predicate *)
Inductive well_typed : expression -> expression -> Prop :=
| WT_Var : forall n, well_typed (Var n) (TType 0)


(* get type *)






(* shallow embedding *)
