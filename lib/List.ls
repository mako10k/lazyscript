{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  .map = \~f -> \~xs -> (
    (
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~rest) -> (~f ~h) : (((~self ~self) ~f ~rest))))
         ~xs))
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~rest) -> (~f ~h) : (((~self ~self) ~f ~rest))))
         ~xs))
      ~f ~xs
    )
  );

  .filter = \~p -> \~xs -> (
    (
      (\~self -> \~p -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~rest) ->
               (((\true  -> (~h : (((~self ~self) ~p ~rest))))
                 | (\false -> (((~self ~self) ~p ~rest))))
                ((~p ~h)))))
         ~xs))
      (\~self -> \~p -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~rest) ->
               (((\true  -> (~h : (((~self ~self) ~p ~rest))))
                 | (\false -> (((~self ~self) ~p ~rest))))
                ((~p ~h)))))
         ~xs))
      ~p ~xs
    )
  );

  .append = \~xs -> \~ys -> (
    (
      (\~self -> \~xs -> \~ys ->
        (((\[] -> ~ys)
          || (\(~h : ~rest) -> (~h : (((~self ~self) ~rest ~ys)))))
         ~xs))
      (\~self -> \~xs -> \~ys ->
        (((\[] -> ~ys)
          || (\(~h : ~rest) -> (~h : (((~self ~self) ~rest ~ys)))))
         ~xs))
      ~xs ~ys
    )
  );

  .reverse = \~xs -> (
    (
      (\~self -> \~acc -> \~zs ->
        (((\[] -> ~acc)
          || (\(~h : ~rest) -> (((~self ~self) (~h : ~acc) ~rest))))
         ~zs))
      (\~self -> \~acc -> \~zs ->
        (((\[] -> ~acc)
          || (\(~h : ~rest) -> (((~self ~self) (~h : ~acc) ~rest))))
         ~zs))
      [] ~xs
    )
  );

  .flatMap = \~f -> \~xs -> (
    (
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          | (\(~h : ~rest) ->
               (
                 ( (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                   (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                 ) ((~f ~h)) (((~self ~self) ~f ~rest))
               )))
         ~xs))
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          | (\(~h : ~rest) ->
               (
                 ( (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                   (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                 ) ((~f ~h)) (((~self ~self) ~f ~rest))
               )))
         ~xs))
      ~f ~xs
    )
  )
};
