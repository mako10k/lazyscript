!{
  ~N <- ~~include "lib/Ns.ls";
  ns <- ({ .a = 1; .b = 2 });
  !((~builtins .dump) (((~N .nsHas) ~ns) .a));
};
