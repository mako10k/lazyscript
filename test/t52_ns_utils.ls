!{
  ~N <- ~~include "lib/Ns.ls";
  ns <- (!!{ .a = 1; .b = 2 });
  !println (~~to_str (((~N .nsHas) ~ns) .a));
  !println (~~to_str ((((~N .nsGetOr) ~ns) .c) 99));
};
