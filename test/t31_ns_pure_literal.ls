!{
  ns = { .Foo = 42; .Inc = (\~n -> (~~add ~n 1)) };
  ~~println (~~to_str (~ns .Foo));
  ~~println (~~to_str ((~ns .Inc) 41));
};
