!{
  ~List = {
    .nil   = [];
    .cons  = \~x -> \~xs -> (~x : ~xs);

    .map = (
      (\~self -> \~f -> \~xs -> (
        \[] -> [] |
        \(~h : ~t) -> (~f ~h) : (((~self ~self ~f)) ~t)
      ) ~xs)
      (\~self -> \~f -> \~xs -> (
        \[] -> [] |
        \(~h : ~t) -> (~f ~h) : (((~self ~self ~f)) ~t)
      ) ~xs)
    );

    .filter = (
      (\~self -> \~p -> \~xs -> (
        \[] -> [] |
        \(~h : ~t) -> (
          ((~p ~h) : (((~self ~self ~p)) ~t))) | ((((~self ~self ~p)) ~t))
        )
      ) ~xs)
      (\~self -> \~p -> \~xs -> (
        \[] -> [] |
        \(~h : ~t) -> (
          ((~p ~h) : (((~self ~self ~p)) ~t))) | ((((~self ~self ~p)) ~t))
        )
      ) ~xs)
    );

    .foldl = (
      (\~self -> \~f -> \~acc -> \~xs -> (
        \[] -> ~acc |
        \(~h : ~t) -> (((~self ~self ~f) ((~f ~acc) ~h)) ~t)
      ) ~xs)
      (\~self -> \~f -> \~acc -> \~xs -> (
        \[] -> ~acc |
        \(~h : ~t) -> (((~self ~self ~f) ((~f ~acc) ~h)) ~t)
      ) ~xs)
    );

    .foldr = (
      (\~self -> \~f -> \~xs -> \~z -> (
        \[] -> ~z |
        \(~h : ~t) -> ((~f ~h) (((~self ~self ~f) ~t) ~z))
      ) ~xs)
      (\~self -> \~f -> \~xs -> \~z -> (
        \[] -> ~z |
        \(~h : ~t) -> ((~f ~h) (((~self ~self ~f) ~t) ~z))
      ) ~xs)
    );

    .append = (\~xs -> \~ys -> (
      \[] -> ~ys | \(~h : ~t) -> (~h : ((~append ~t) ~ys))
    ) ~xs);

    .reverse = (((.foldl) (\~acc -> \~x -> (~x : ~acc))) []);

    .flatMap = (\~f -> ((.foldr) (\~x -> \~acc -> ((.append) (~f ~x) ~acc)) []))
  };
};
