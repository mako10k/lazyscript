{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  {- fixed point combinator -}
  ~fix <- \~f -> (\~x -> ~f (~x ~x)) (\~x -> ~f (~x ~x));

  .map = ~fix (\~rec ~f -> (\[] -> []) | (~h : ~t) -> ((~f ~h) : (~rec ~t)));

  .filter = (\~fix -> (\~p -> (
    ((~fix (\~rec -> \~xs -> (
      (((\[] -> []) | (\(~h : ~t) -> ((((\true -> (~h : ((~rec) ~t))) | (\false -> ((~rec) ~t))) ((~p ~h))))) ~xs)
    )))
  ))) (((~prelude nsSelf) .fix));

  .append = (\~fix -> (\~xs -> \~ys -> (
    (((~fix (\~rec -> \~xs2 -> (
      (((\[] -> ~ys) | (\(~h : ~t) -> (~h : ((~rec) ~t)))) ~xs2)
    ))) ~xs)
  ))) (((~prelude nsSelf) .fix));

  .reverse = (\~fix -> (\~xs -> (
    ((((~fix (\~rec -> \~acc -> \~xs2 -> (
      (((\[] -> ~acc) | (\(~h : ~t) -> ((~rec) (~h : ~acc) ~t))) ~xs2)
    ))) []) ~xs)
  ))) (((~prelude nsSelf) .fix));

  .flatMap = (\~append -> \~fix -> (\~f -> (
    ((~fix (\~rec -> \~xs -> (
      (((\[] -> []) | (\(~h : ~t) -> ((~append (~f ~h)) ((~rec) ~t)))) ~xs)
    )))
  ))) (((~prelude nsSelf) .append)) (((~prelude nsSelf) .fix));
};
