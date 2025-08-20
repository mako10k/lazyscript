!{
  ~~require "lib/ns.ls";
  ~ns <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns .a 1);
  ((~prelude nsdefv) ~ns .b 2);
  ~~println (~to_str (((~nsHas ~ns) .a)));
  ~~println (~to_str ((((~nsGetOr ~ns) .c) 99)));
};
