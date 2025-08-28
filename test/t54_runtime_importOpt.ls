!{
  ~ns  <- { .x = 1; .y = 2 };
  ~bad <- (~Prelude .env .importOpt) 123;  # invalid, should yield None
  ~ok  <- (~Prelude .env .importOpt) ~ns;  # valid, yields Some () and imports x,y
  ~s1  <- (~Prelude .to_str) (~Prelude .Option ~bad) ~bad;
  ~s2  <- (~Prelude .to_str) (~Prelude .Option ~ok) ~ok;
  ~sum <- (~~add ~x ~y);  # if import succeeded, x and y are in scope
  ~~strcat ~s1 (~~strcat "|" (~~strcat ~s2 (~~strcat "|" ((~Prelude .to_str) ~sum))))
};
