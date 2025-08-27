!{
  ~N <- ~~include "lib/Ns.ls";
  ns <- ({ .a = 1; .b = 2 });
  v1 <- (((~N .nsHas) ~ns) .a);
  !(~~trace "v1" v1);
  !println (~~to_str v1);
  v2 <- ((((~N .nsGetOr) ~ns) .c) 99);
  !(~~trace "v2" v2);
  !println (~~to_str v2);
};
