{
  .nsMembers = (~~nsMembers);

  # nsHas: nsMembers を走査して一致を判定（List.ls と同型の自己適用再帰）
  .nsHas = \~ns -> \~k -> (
    (
      (\~self -> \~xs ->
         (((\[] -> false)
           || (\(~h : ~t) ->
                 ( ((\true  -> true)
                    | (\false -> ((~self ~self) ~t)))
                   ((~~eq) ~h ~k) )))
          ~xs))
      (\~self -> \~xs ->
         (((\[] -> false)
           || (\(~h : ~t) ->
                 ( ((\true  -> true)
                    | (\false -> ((~self ~self) ~t)))
                   ((~~eq) ~h ~k) )))
          ~xs))
      ((~~nsMembers) ~ns)
    )
  );

  # nsGetOr: 見つかれば (~ns ~k)、無ければ ~d（自己適用再帰）
  .nsGetOr = \~ns -> \~k -> \~d -> (
    (
      (\~self -> \~xs ->
         (((\[] -> ~d)
           || (\(~h : ~t) ->
                 ( ((\true  -> (~ns ~k))
                    | (\false -> ((~self ~self) ~t)))
                   ((~~eq) ~h ~k) )))
          ~xs))
      (\~self -> \~xs ->
         (((\[] -> ~d)
           || (\(~h : ~t) ->
                 ( ((\true  -> (~ns ~k))
                    | (\false -> ((~self ~self) ~t)))
                   ((~~eq) ~h ~k) )))
          ~xs))
      ((~~nsMembers) ~ns)
    )
  );
};
