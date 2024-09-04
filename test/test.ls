#!/usr/local/bin/lazyscript
{-
-}
{
  ~print "abc";
  {- \~args -> ~go ~args; -}
  ~go = \[] -> ~prelude exit 0 |
  \(~hd : ~tl) -> ~prelude chain (~prelude println hd) \() -> ~go ~tl;
  ~join = \~m -> ~go chain ~m ~id;
  ~id = \~x -> ~x;
  ~fix = \~f -> {
    ~x;
    ~x = ~f ~x
  };
  ~prelude = prelude;
  ~chain = \~m -> \~f -> ~join (~fmap ~f ~m);
  ~fmap = \~f -> \~m -> ~chain ~m \~x -> ~return (~f ~x);
  ~pred = \(Cons ~x ~y) -> ~y;
  ~fib = ~fix \~f -> {
    \0 -> 1 |
    \1 -> 1 |
    \(Int ~n) -> {
      ~add (~f ~n_1) (~f ~n_2);
      ~n_1 = ~pred ~n;
      ~n_2 = ~pred ~n_1
    }
  };
  ~add = \~x -> succ ~x;
  ~f = \~x -> \~x -> ~x;
  ~return = \~x -> return ~x;
  ~testList = \[~x, ~y, ~z] -> ~fmap (~add 1) [1, 2, 3, 4];
  ~testNil = \[] -> [];
  ~testCons = \(~x : ~y : ~z : []) -> (((~x : ~y) : ~z) : []);
  ~testTuple = \() -> () |
  \(~x) -> ~x |
  \(~x, ~y) -> (~x, ~y) |
  \(~x, ~y, ~z) -> (~x, ~y, ~z)
};