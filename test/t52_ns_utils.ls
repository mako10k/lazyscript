!{
  ~N <- ((~prelude requirePure) "lib/ns.ls");
  ~ns <- ((~prelude nsnew0));
  ~~nsdefv ~ns .a 1;
  ~~nsdefv ~ns .b 2;
  ~~println (~to_str ((((~N .nsHas) ~ns) .a)));
  ~~println (~to_str (((~((~N .nsGetOr) ~ns)) .c) 99));
};
