~g ~x;

~f = \~x -> alge ~x ~y;

~g = \(~a, ~b) -> ~seq ~a ~b; 

~x = (~y, ~z);

~y = ~f ~z;

~z = ~f ~y;

--------------------
PREPARE:

r01: ~g := bind_target(bind = `~g = \(~a, ~b) -> ~seq ~a ~b`, ref = `~g` )
r02:   ~seq := thunk_target(thunk = builtin_seq)
r03:   ~a := lambda_target(lambda = `(~a, ~b)`, ref = `~a`)
r04:   ~b := lambda_target(lambda = `(~a, ~b)`, ref = `~b`)
r05: ~x := bind_target(bind = `~x = (~y, ~z)`, ref = `~x`)
r06:  ~y := bind_target(bind = `~y = ~f ~z`, ref = `~y`)
r07:    ~f := bind_target(bind = `~f = ~x -> alge ~x ~y`, ref = `~f`)
r08:      ~x := lambda_tatget(lambda = `~x`, ref = `~x`)
          ~y := r06
r09:    ~z := bind_target(bind = `~z = ~f ~y`, ref = `~z`)
          ~f := r07
          ~y := r06
----------------------
EVAL:

eval: ~g ~x
  eval: ~g
    match: ~g <-> \(~a, ~b) -> ~seq ~a ~b ==> m01: ~g: \(~a, ~b) -> ~seq ~a ~b
  ==> \(~a, ~b) -> ~seq ~a ~b
==> (\(~a, ~b) -> ~seq ~a ~b) ~x
eval: (\(~a, ~b) -> ~seq ~a ~b) ~x
  match: (~a, ~b) <-> ~x
    eval: ~x
      match: ~x <-> (~y, ~z) ==> m02: ~x: (~y, ~z)
    ==> (~y, ~z)
  ==> m03: ~a: ~y, ~b: ~z
  beta: ~seq ~a ~b / m03 ==> ~seq ~y ~z
==> ~seq ~y ~z
eval: ~seq => builtin_seq/2
eval: builtin_seq/2 ~y ~z
  eval: ~y
    match: ~y <-> ~f ~z ==> m04: ~y: ~f ~z
    beta: ~y / m04 ==> ~f ~z
    eval: ~f ~z
      eval: ~f => 
  eval: ~z
    match: ~z <-> ~f ~y ==> m05: ~z: ~f ~y
    beta: ~z / m05 ==> ~f ~y


