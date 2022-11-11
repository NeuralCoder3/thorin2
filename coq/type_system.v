Require Import List.
Import ListNotations.
(* Load defs. *)

(* deep embedding *)

Definition name := nat.

(* or use referencing (var) for lam *)
  (* Unset Positivity Checking. *)

(* types are also expressions *)
Inductive expression :=
| Var : nat -> expression
(*      name *)
| Nat : expression
| App : expression -> expression -> expression
(*      callee        args *)
| Arr : name -> expression -> expression -> expression
(*              shape         body *)
| Extract: expression -> expression -> expression
(*         tuple         index *)
| Pack : name -> expression -> expression -> expression
(*               shape         body *)
| Insert : expression -> expression -> expression -> expression
(*       tuple         index            value *)
| Idx : expression -> expression
(*       width *)
| Lit : expression -> nat -> expression
(*      type          value *)
| Pi : name -> expression -> expression -> expression
(*             dom         codom *)
| TType : nat -> expression
(* | Star | Box *)
(* TODO: is this star and box? *)
(*        level *)
| Sigma : list expression -> expression
(*        type fields *)
| Tuple : list expression -> expression
(*        expression fields *)
(* TODO: needs type? *)

(* nom *)
| Lam : name -> expression -> expression-> expression -> expression
(*              type          filter       body *)
(* | nom_Sigma :  *)
(* | nom_Pack :  *)
(* TODO: nom Pi *)

(* 
Axiom
*)

(*
Pick, Proxy, Singelton, Test, Univ, Vel
*)
.

Definition Star := TType 0.
Definition Box := TType 1.

  Set Positivity Checking.

(*
https://github.com/AnyDSL/thorin2/blob/master/thorin/def.cpp#L73
*)


(* compare thorin paper *)



(* type predicate *)

Definition env := list (name * expression * option expression).
(* name, type, option value *)

Definition validType s :=
  match s with
  | TType _ => True
  | _ => False
  end.

(* TODO: use notation *)
Inductive well_typed (Γ:env) : expression -> expression -> Prop :=
(* | Type_Star: well_typed Γ Star Box *)
| Typed_Level n: well_typed Γ (TType n) (TType (S n))
| Typed_Nat: well_typed Γ Nat Star
| Typed_Lit t n: well_typed Γ (Lit t n) t
| Typed_Idx e: well_typed Γ e Nat -> well_typed Γ (Idx e) Star
| Typed_Var n t e:
    In (n, t, e) Γ ->
    well_typed Γ (Var n) t
(* let does not exist *)
(* TODO: compare Coq typing universe rule *)
| Typed_Pi x t u st su: 
    well_typed Γ t st ->
    well_typed ((x, t, None)::Γ) u su ->
    validType su -> validType st ->
    well_typed Γ (Pi x t u) (su)
| Typed_Lam n t f e u:
    (* TODO: filter *)
    well_typed ((n, t, None)::Γ) e u ->
    well_typed Γ (Lam n t f e) (Pi n t e)
(* | Typed_App e1 e2 t1 t2: *)
  (* TODO: substitution *)
(* TODO: Sig, Tuple, Extract, *)
| Typed_Arr x en e s s':
    well_typed Γ en Nat ->
    well_typed ((x, Idx en, None)::Γ) e s ->
    (* TODO: s as type list is bound by s' (denoted with ~>) *)
    well_typed Γ (Arr x en e) s'
| Typed_Pack x en e t:
    well_typed Γ en Nat ->
    well_typed ((x, Idx en, None)::Γ) e t ->
    well_typed Γ (Pack x en e) (Arr x en t)

(* Axiom *)
.



(* get type *)






(* shallow embedding *)
