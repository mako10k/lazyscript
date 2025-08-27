!{
  !println (~~to_str ((((\true -> 1) | (\false -> 2)) ((~~eq .a .c)))));
  !println (~~to_str ((((\true -> 1) | (\false -> 2)) ((~~eq .a .a)))));
};
