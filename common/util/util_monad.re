open Util_kernel;

/* Convenience module type */
module type Functor = {
  type t 'a;
  let map : ('a => 'b) => t 'a => t 'b;
};

/* Base monadic module types */
module type Monad_base = {
  type t 'a;
  let (>>=) : t 'a => ('a => t 'b) => t 'b;
  let pure : 'a => t 'a;
  let map_impl : [ `Define_using_bind | `Custom of ('a => 'b) => t 'a => t 'b ];
};

module type Monad_plus_base = {
  include Monad_base;
  let mzero : t 'a;
  let mplus : t 'a => t 'a => t 'a;
};

/* Full monadic types */
module type Monad = {
  include Monad_base;
  let (>>) : t 'a => t 'b => t 'b;
  let (=<<) : ('a => t 'b) => t 'a => t 'b;

  let map : ('a => 'b) => t 'a => t 'b;
  let (<$>) : ('a => 'b) => t 'a => t 'b;

  let ap : t ('a => 'b) => t 'a => t 'b;
  let (<->) : t ('a => 'b) => t 'a => t 'b;

  let sequence : list (t 'a) => t 'a;
  let sequence_ : list (t 'a) => t unit;

  let mapM : ('a => t 'b) => list 'a => t (list 'b);
  let mapM_ : ('a => t 'b) => list 'a => t unit;
};

module type Monad_plus = {
  include Monad;
  include Monad_plus_base with type t 'a := t 'a;
  let msum : list (t 'a) => t 'a;
  let mfilter : ('a => bool) => t 'a => t 'a;
};

/* Functors to add combinators to monads */
module Monad_make = fun (M: Monad_base) => {
  open M;

  let bind = (>>=);
  let (>>) a b => a >>= fun _ => b;
  let (=<<) f v => v >>= f;

  let map = switch map_impl {
    | `Define_using_bind => fun f mx => mx >>= fun x => pure @@ f x
    | `Custom f => f
  };
  let (<$>) = map;
  let (|$>) v f => map f v;

  let ap mf mx =>
    mf >>= flip map mx;
  let (<->) = ap;
  let (|->) v f => ap f v;

  let sequence l => {
    let k m m' =>
      m >>= fun v =>
      m' >>= fun v' =>
      pure [v, ...v'];
    fold_right k l @@ pure [];
  };
  let sequence_ l => fold_right (>>) l (pure ());

  let mapM f l => Util_kernel.map f l |> sequence;
  let mapM_ f l => Util_kernel.map f l |> sequence_;
};

module Monad_plus_make = fun (M: Monad_plus_base) => {
  open M;
  let msum l => fold_left mplus mzero l;
  let filter f m => m >>= fun v => f v ? pure v : mzero;
};
