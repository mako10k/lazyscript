!{
  v <- (((\true->1) | (\false->0)) (((~prelude eq)) .a .a));
  !println (~~to_str v);
};
