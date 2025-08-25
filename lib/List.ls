{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  .map = \~f -> \~xs -> (
    (
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~t) -> (~f ~h) : (((~self ~self) ~f ~t))))
         ~xs))
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~t) -> (~f ~h) : (((~self ~self) ~f ~t))))
         ~xs))
      ~f ~xs
    )
  );

  .filter = \~p -> \~xs -> (
    (
      (\~self -> \~p -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~t) ->
               (((\true  -> (~h : (((~self ~self) ~p ~t))))
                 | (\false -> (((~self ~self) ~p ~t))))
                ((~p ~h)))))
         ~xs))
      (\~self -> \~p -> \~xs ->
        (((\[] -> [])
          || (\(~h : ~t) ->
               (((\true  -> (~h : (((~self ~self) ~p ~t))))
                 | (\false -> (((~self ~self) ~p ~t))))
                ((~p ~h)))))
         ~xs))
      ~p ~xs
    )
  );

  .append = \~xs -> \~ys -> (
    (
      (\~self -> \~xs -> \~ys ->
        (((\[] -> ~ys)
          || (\(~h : ~t) -> (~h : (((~self ~self) ~t ~ys)))))
         ~xs))
      (\~self -> \~xs -> \~ys ->
        (((\[] -> ~ys)
          || (\(~h : ~t) -> (~h : (((~self ~self) ~t ~ys)))))
         ~xs))
      ~xs ~ys
    )
  );

  .reverse = \~xs -> (
    (
      (\~self -> \~acc -> \~zs ->
        (((\[] -> ~acc)
          || (\(~h : ~t) -> (((~self ~self) (~h : ~acc) ~t))))
         ~zs))
      (\~self -> \~acc -> \~zs ->
        (((\[] -> ~acc)
          || (\(~h : ~t) -> (((~self ~self) (~h : ~acc) ~t))))
         ~zs))
      [] ~xs
    )
  );

  .flatMap = \~f -> \~xs -> (
    (
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          | (\(~h : ~t) ->
               (
                 ( (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                   (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                 ) ((~f ~h)) (((~self ~self) ~f ~t))
               )))
         ~xs))
      (\~self -> \~f -> \~xs ->
        (((\[] -> [])
          | (\(~h : ~t) ->
               (
                 ( (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                   (\~apSelf -> \~as -> \~bs ->
                       ((((\[] -> ~bs)
                          || (\(~h1 : ~t1) -> (~h1 : (((~apSelf ~apSelf) ~t1 ~bs)))))
                        ) ~as))
                 ) ((~f ~h)) (((~self ~self) ~f ~t))
               )))
         ~xs))
      ~f ~xs
    )
  )
};
