{
  .nsMembers = (~~nsMembers);

  .nsHas = \~ns -> \~k -> (
    (
      \~self -> \~xs ->
        ((\[] -> false)
          || (\(~h : ~t) ->
                (((\true -> true)
                  | (\false -> (~self ~self ~t)))
                 ((~~eq) ~h ~k))))
        ~xs
    )
    (\~self -> \~xs ->
        ((\[] -> false)
          || (\(~h : ~t) ->
                (((\true -> true)
                  | (\false -> (~self ~self ~t)))
                 ((~~eq) ~h ~k))))
        ~xs)
  ((~~nsMembers) ~ns)
  );

  .nsGetOr = \~ns -> \~k -> \~d -> (
    (
      \~self -> \~xs ->
        ((\[] -> ~d)
          || (\(~h : ~t) ->
                (((\true  -> (~ns ~k))
                  | (\false -> (~self ~self ~t)))
                 ((~~eq) ~h ~k))))
        ~xs
    )
    (\~self -> \~xs ->
        ((\[] -> ~d)
          || (\(~h : ~t) ->
                (((\true  -> (~ns ~k))
                  | (\false -> (~self ~self ~t)))
                 ((~~eq) ~h ~k))))
        ~xs)
  ((~~nsMembers) ~ns)
  );
};
