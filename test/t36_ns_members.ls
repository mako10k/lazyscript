!{
  ns <- !nsnew0;
  !nsdefv ~ns .a 1;
  !nsdefv ~ns .b 2;
  !nsdefv ~ns .aa 3;
  !nsdefv ~ns .ab 4;
  ~~println (~~to_str ((~prelude nsMembers) ~ns));
};
