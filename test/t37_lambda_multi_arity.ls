!{
  !println (~~to_str (((\ ~x ~y -> (~~add ~x ~y)) 3) 4));
  !println (~~to_str (((\ Foo _ -> 99) Foo) 0));
};
