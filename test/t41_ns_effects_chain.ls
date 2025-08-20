!{
  ns <- ((~prelude nsnew0));
  ((~prelude chain) ((~prelude nsdefv) ~ns .x 1) (\~_ -> ()));
  ((~prelude nsMembers) ~ns)
};
