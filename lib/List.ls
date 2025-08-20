{
  .nil   = [];
  .cons  = \~x -> \~xs -> (~x : ~xs);

  .foldl = (
    (\~self -> \~f -> \~acc -> \~xs -> (
      \[] -> ~acc |
      \(~h : ~t) -> (((~self ~self) ~f ((~f ~acc) ~h) ~t))
    ) ~xs)
    (\~self -> \~f -> \~acc -> \~xs -> (
      \[] -> ~acc |
      \(~h : ~t) -> (((~self ~self) ~f ((~f ~acc) ~h) ~t))
    ) ~xs)
  );

  .foldr = (
    (\~self -> \~f -> \~xs -> \~z -> (
      \[] -> ~z |
      \(~h : ~t) -> ((~f ~h) (((~self ~self) ~f ~t ~z)))
    ) ~xs)
    (\~self -> \~f -> \~xs -> \~z -> (
      \[] -> ~z |
      \(~h : ~t) -> ((~f ~h) (((~self ~self) ~f ~t ~z)))
    ) ~xs)
  );

  .map = (\~f -> ((.foldr) (\~x -> \~acc -> ((~f ~x) : ~acc)) []));

  .filter = (
    (\~self -> \~p -> \~xs -> (
      \[] -> [] |
      \(~h : ~t) -> (
        ((~p ~h) : (((~self ~self) ~p ~t))) | (((~self ~self) ~p ~t))
      )
    ) ~xs)
    (\~self -> \~p -> \~xs -> (
      \[] -> [] |
      \(~h : ~t) -> (
        ((~p ~h) : (((~self ~self) ~p ~t))) | (((~self ~self) ~p ~t))
      )
    ) ~xs)
  );

  .append = (
    (\~self -> \~xs -> \~ys -> (
      \[] -> ~ys |
      \(~h : ~t) -> (~h : (((~self ~self) ~t ~ys)))
    ) ~xs)
    (\~self -> \~xs -> \~ys -> (
      \[] -> ~ys |
      \(~h : ~t) -> (~h : (((~self ~self) ~t ~ys)))
    ) ~xs)
  );

  .reverse = (((.foldl) (\~acc -> \~x -> (~x : ~acc)) []));

  .flatMap = (\~f -> ((.foldr) (\~x -> \~acc -> ((.append) (~f ~x) ~acc)) []));
};
