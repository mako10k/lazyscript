!{
  ns <- !nsnew0;
  !nsdefv ns .Foo 42;
  !nsdefv ns .Inc (\~n -> ~~add ~n 1);
  ~~println (~~to_str (~ns .Foo));
  ~~println (~~to_str ((~ns .Inc) 41));
};
