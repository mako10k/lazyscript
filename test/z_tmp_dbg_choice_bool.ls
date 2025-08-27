!{
  v <- (((~prelude eq)) .a .a);
  !println (~~to_str v);
  w <- (((\true->1) | (\false->0)) v);
  !println (~~to_str w);
};
