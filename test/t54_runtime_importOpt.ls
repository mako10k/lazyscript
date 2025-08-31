!{
  ~ns  <- { .x = 1; .y = 2 };
  ~bad <- (~prelude .env .importOpt) 123;  # invalid, should yield None
  ~ok  <- (~prelude .env .importOpt) ~ns;  # valid, yields Some () and imports x,y
  ~s1  <- (~prelude .to_str) (~prelude .Option ~bad) ~bad;
  ~s2  <- (~prelude .to_str) (~prelude .Option ~ok) ~ok;
  ~sum <- (~~add ~x ~y);  # if import succeeded, x and y are in scope
  ~~strcat ~s1 (~~strcat "|" (~~strcat ~s2 (~~strcat "|" ((~prelude .to_str) ~sum))))
};
