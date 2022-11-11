Require Import Vector.
Import VectorNotations.


(* Definition Idx (n:nat) := nat. *)
Definition Idx (n:nat) := Fin.t n.
Definition Cont := Prop.
Definition Cn (A:Type) := A -> Cont.
Notation Bool := (Idx 2).
Definition _32 := 4294967296.
Notation I32 := (Idx _32).
Notation vector := (Vector.t).
Definition Unit := True.

Definition heterogen_vector := 
  vector {T:Type & T}.
