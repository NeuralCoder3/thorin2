Load helper.

(* Definition get_nat {n:nat} (a:Fin.t n) : nat := proj1_sig (Fin.to_nat a). *)
(* Transparent get_nat. *)
Notation get_nat a := (proj1_sig (Fin.to_nat a)).

Lemma mod_lift_unfold f n ab:
    let '(a,b) := ab in
    let an := get_nat a in
    let bn := get_nat b in
    let r := (f an bn) mod n in
exists 
    (Hr: r < n),
    @mod_lift f n (a,b) = Fin.of_nat_lt Hr.
Proof.
    cbv zeta.
    destruct ab.
    unfold mod_lift, get_nat.
    destruct (Fin.to_nat).
    destruct (Fin.to_nat).
    eexists.
    reflexivity.
Qed.

Lemma mod_lift_prop f n a b:
    get_nat (@mod_lift f n (a,b)) = (f (get_nat a) (get_nat b)) mod (n).
Proof.
(* or use mod_lift_unfold directly *)
    unfold mod_lift, get_nat.
    destruct (Fin.to_nat a).
    destruct (Fin.to_nat b).
    rewrite Fin.to_nat_of_nat.
    cbn.
    reflexivity.
Qed.
