import Std

/-
Selected formal obligations for arXiv:2603.24880v2.
Lean 4.19.0; no Mathlib, additional axioms, sorry, or native_decide.
This file does NOT prove the Four Color Theorem or Theorem 1.1.
-/
namespace PaperAudit

-- Arithmetic consequences, conditional on the stated graph/charge hypotheses.
theorem euler_charge (n m totalDegree : Int)
    (edges : m = 3*n - 6) (handshake : totalDegree = 2*m) :
    60*n - 10*totalDegree = 120 := by omega

theorem charge_upper_bound (d incoming outgoing finalCharge : Int)
    (hin : incoming ≤ 8*d) (hout : 0 ≤ outgoing)
    (hcharge : finalCharge = 10*(6-d) + incoming - outgoing) :
    finalCharge ≤ 60 - 2*d := by omega

theorem nonnegative_degree_bound (d t : Int)
    (h : t ≤ 60 - 2*d) (ht : 0 ≤ t) : d ≤ 30 := by omega

theorem negative_degree_bound (d t : Int)
    (ht : t < 0) (h : t ≤ 60 - 2*d) : d ≤ -31*t := by omega

-- Section 13.3: the text's "b ≤ 100b" must use 100*b ≤ c.
theorem cycle_savings (c c1 c2 b : Int)
    (h1 : 0 ≤ c1) (h2 : 0 ≤ c2) (_hb : 0 ≤ b)
    (hcount : c = c1 + c2 + b) (hbig : 100*b ≤ c) :
    c ≤ 2*(c1 + 2*c2 - 12*b) := by omega

-- Conditional expectation: one binary choice preserves the current potential.
theorem conditional_choice (before yes no : Int)
    (h : yes + no = 2*before) : before ≤ yes ∨ before ≤ no := by omega

def swap (a b x : Fin 4) : Fin 4 := if x = a then b else if x = b then a else x
def recolor (a b x : Fin 4) (selected : Bool) : Fin 4 :=
  if selected then swap a b x else x

-- Exact finite color algebra. Closure is needed only on bichromatic edges.
theorem kempe_edge : ∀ (a b x y : Fin 4) (s t : Bool),
    x ≠ y →
    ((x = a ∨ x = b) → (y = a ∨ y = b) → s = t) →
    recolor a b x s ≠ recolor a b y t := by decide

-- Applies to arbitrary vertex types and edge relations, not just small graphs.
theorem kempe_preserves_proper {V : Type} (edge : V → V → Prop)
    (color : V → Fin 4) (selected : V → Bool) (a b : Fin 4)
    (proper : ∀ u v, edge u v → color u ≠ color v)
    (closed : ∀ u v, edge u v →
      (color u = a ∨ color u = b) → (color v = a ∨ color v = b) →
      selected u = selected v) :
    ∀ u v, edge u v →
      recolor a b (color u) (selected u) ≠ recolor a b (color v) (selected v) := by
  intro u v huv
  exact kempe_edge a b (color u) (color v) (selected u) (selected v)
    (proper u v huv) (closed u v huv)

-- Octahedron: vertices 0 and 1 are poles; 2,3,4,5 form its equatorial C4.
def octEdge (u v : Fin 6) : Prop :=
  u ≠ v ∧ ((u.val < 2 ∧ 2 ≤ v.val) ∨ (v.val < 2 ∧ 2 ≤ u.val) ∨
    (u.val = 2 ∧ v.val = 3) ∨ (u.val = 3 ∧ v.val = 2) ∨
    (u.val = 3 ∧ v.val = 4) ∨ (u.val = 4 ∧ v.val = 3) ∨
    (u.val = 4 ∧ v.val = 5) ∨ (u.val = 5 ∧ v.val = 4) ∨
    (u.val = 5 ∧ v.val = 2) ∨ (u.val = 2 ∧ v.val = 5))

instance (u v : Fin 6) : Decidable (octEdge u v) := inferInstanceAs (Decidable (_ ∧ _))

theorem poles_non_touching : (0 : Fin 6) ≠ 1 ∧ ¬ octEdge 0 1 := by decide
theorem poles_same_ring : ∀ v : Fin 6, octEdge 0 v ↔ octEdge 1 v := by decide
theorem four_ring_vertices : ∀ v : Fin 6, octEdge 0 v ↔ 2 ≤ v.val := by decide
theorem overlapping_rings : ∃ v : Fin 6, octEdge 0 v ∧ octEdge 1 v := by
  exact ⟨2, by decide⟩

-- Dart formalism from §9.2. Along uses only successor/predecessor, not reverse.
structure DartRep (V D : Type) where
  head : D → V
  rev : D → D
  succ : D → Option D
  pred : D → Option D

inductive Along {D : Type} (s p : D → Option D) : D → D → Prop
  | refl (a) : Along s p a a
  | step {a b c} : (s a = some b ∨ p a = some b) → Along s p b c → Along s p a c

def M1 {V D} (z : DartRep V D) : Prop := ∀ v, ∃ d, z.head d = v
def M2 {V D} (z : DartRep V D) : Prop := ∀ d, z.rev (z.rev d) = d
def M3 {V D} (z : DartRep V D) : Prop :=
  ∀ e f, z.pred f = some e ↔ z.succ e = some f
def M4 {V D} (z : DartRep V D) : Prop :=
  ∀ e f, z.succ e = some f → z.head e = z.head f
def face {V D} (z : DartRep V D) (e : D) : Option D := (z.succ e).map z.rev
def M5 {V D} (z : DartRep V D) : Prop :=
  ∀ e, z.succ e ≠ none → ((face z e).bind (face z)).bind (face z) = some e
def M6 {V D} (z : DartRep V D) : Prop :=
  ∀ e f, z.head e = z.head f → Along z.succ z.pred e f

def source : DartRep Bool Bool := ⟨id, Bool.not, fun _ => none, fun _ => none⟩
def target : DartRep Unit Bool := ⟨fun _ => (), Bool.not, fun _ => none, fun _ => none⟩

def Hom {V D V' D'} (z : DartRep V D) (w : DartRep V' D')
    (fv : V → V') (fd : D → D') : Prop :=
  (∀ d, w.head (fd d) = fv (z.head d)) ∧
  (∀ d, w.rev (fd d) = fd (z.rev d)) ∧
  (∀ e f, z.succ e = some f → w.succ (fd e) = some (fd f)) ∧
  (∀ e f, z.pred e = some f → w.pred (fd e) = some (fd f))

def MinimalImage {V D V' D'} (z : DartRep V D) (w : DartRep V' D')
    (fv : V → V') (fd : D → D') : Prop :=
  Hom z w fv fd ∧
  (∀ v', ∃ v, fv v = v') ∧ (∀ d', ∃ d, fd d = d') ∧
  (∀ d', w.succ d' ≠ none ↔ ∃ d, fd d = d' ∧ z.succ d ≠ none) ∧
  (∀ d', w.pred d' ≠ none ↔ ∃ d, fd d = d' ∧ z.pred d ≠ none)

theorem source_basic : M1 source ∧ M2 source ∧ M3 source ∧ M4 source ∧ M5 source := by
  simp [M1, M2, M3, M4, M5, source, Bool.forall_bool, Bool.exists_bool]
theorem source_single_list : M6 source := by
  intro e f h
  have hef : e = f := h
  subst f
  exact Along.refl e
theorem target_basic : M1 target ∧ M2 target ∧ M3 target ∧ M4 target ∧ M5 target := by
  simp [M1, M2, M3, M4, M5, target, Bool.forall_bool, Bool.exists_bool]
theorem target_minimal : MinimalImage source target (fun _ => ()) id := by
  simp [MinimalImage, Hom, source, target, Bool.forall_bool, Bool.exists_bool]

theorem along_nil {D : Type} {a b : D}
    (h : Along (fun _ => none) (fun _ => none) a b) : a = b := by
  cases h with
  | refl => rfl
  | step h _ => cases h with
    | inl h => cases h
    | inr h => cases h

-- Counterexample to Lemma 9.3 as printed: M6 is not preserved by every minimal image.
theorem target_not_single_list : ¬ M6 target := by
  intro h
  have path := h false true rfl
  have eq : false = true := along_nil path
  cases eq

#print axioms euler_charge
#print axioms charge_upper_bound
#print axioms nonnegative_degree_bound
#print axioms negative_degree_bound
#print axioms cycle_savings
#print axioms conditional_choice
#print axioms kempe_edge
#print axioms kempe_preserves_proper
#print axioms poles_non_touching
#print axioms poles_same_ring
#print axioms four_ring_vertices
#print axioms overlapping_rings
#print axioms source_basic
#print axioms source_single_list
#print axioms target_basic
#print axioms target_minimal
#print axioms target_not_single_list
end PaperAudit
