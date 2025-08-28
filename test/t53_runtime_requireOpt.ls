!{
  ~res  <- !requireOpt "no_such_file.ls";
  ~res2 <- !requireOpt "lib_req.ls";
  ~~strcat ((~Prelude .to_str) (~Prelude .Option ~res) ~res)
          (~~strcat "|" ((~Prelude .to_str) (~Prelude .Option ~res2) ~res2))
};
