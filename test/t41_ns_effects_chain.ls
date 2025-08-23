!{
  ns <- ((~prelude nsnew0));
  ((~prelude chain) (~~nsdefv ~ns .x 1) (\~_ -> ()));
  (~~nsMembers ~ns)
};
