{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  {- fixed point combinator -}
  ~fix <- \~f -> (\~x -> ~f (~x ~x)) (\~x -> ~f (~x ~x));

  .map = ~fix (\~rec ~f ~xs -> (
    \[] -> [] |
    \(~h : ~t) -> (~f ~h) : (~rec ~f ~t)
  ) ~xs);

  .filter = ~fix (\~rec ~p ~xs -> (
    \[] -> [] |
    \(~h : ~t) ->
      (
        (\true  -> ~h : ~rec ~p ~t) |
        (\false ->      ~rec ~p ~t)
      ) (~p ~h)
  ) ~xs);

  .append = ~fix (\~rec ~xs ~ys -> (
    \[] -> ~ys |
    \(~h : ~t) -> ~h : ~rec ~t ~ys
  ) ~xs);

  .reverse = ~fix (\~rec ~acc ~xs -> (
    \[] -> ~acc |
    \(~h : ~t) -> ~rec (~h : ~acc) ~t
  ) ~xs) [];

  .flatMap = ~fix (\~rec ~f ~xs -> (
    \[] -> [] |
    \(~h : ~t) -> ~append (~f ~h) (~rec ~f ~t)
  ) ~xs);

};
