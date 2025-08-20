!{
  ns <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns .a 1);
  ((~prelude nsdefv) ~ns .b 2);
  ((~prelude nsdefv) ~ns "aa" 3);
  ((~prelude nsdefv) ~ns "ab" 4);
  ((~prelude nsdefv) ~ns 2 5);
  ((~prelude nsdefv) ~ns 10 6);
  ((~prelude nsdefv) ~ns Foo 7);
  ((~prelude nsdefv) ~ns Bar 8);
  ~~println (~to_str ((~prelude nsMembers) ~ns));
};
