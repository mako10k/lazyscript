{
  .nsHas ~ns ~m = (
    ~has ~ns;
    ~has = (
      \[] -> false |
      \(~h : ~t) -> (((\true -> true) | (\false -> (~has ~t))) ((~~eq ~h ~m)))
    )
  );
};
!{
  N <- ~~include "test/z_min_ns_has.ls";
  ns <- (.a : (.b : []));
  !println (~~to_str (((~N .nsHas) ~ns) .a));
  !println (~~to_str (((~N .nsHas) ~ns) .c));
};
