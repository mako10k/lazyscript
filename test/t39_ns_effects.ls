!{
  ns1 <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns1 .a 1);
  ~~println (~to_str ((~prelude nsMembers) ~ns1));
};
