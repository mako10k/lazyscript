!{
  let ns = { .x = 1; .y = 2 };
  let bad = (~Prelude .env .importOpt) 123;  # invalid, should yield None
  let ok  = (~Prelude .env .importOpt) ~ns;  # valid, yields Some () and imports x,y
  let s1 = (~Prelude .to_str) (~Prelude .Option bad) ~bad;
  let s2 = (~Prelude .to_str) (~Prelude .Option ok) ~ok;
  let sum = ~x + ~y;  # if import succeeded, x and y are in scope
  s1 ++ "|" ++ s2 ++ "|" ++ (~Prelude .to_str) ~sum
};
