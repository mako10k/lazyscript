!{
  ~res  <- !requireOpt "no_such_file.ls";
  ~res2 <- !requireOpt "lib_req.ls";
  ~~strcat (~~to_str ((~prelude .Option ~res) ~res))
          (~~strcat "|" (~~to_str ((~prelude .Option ~res2) ~res2)))
};
