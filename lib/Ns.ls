{
  .nsMembers = (~~nsMembers);

  # nsHas: メンバー列に対する構造的再帰（prelude.fix を使用）
  .nsHas = \~ns -> \~k -> (
    ((~~fix)
      (\~rec -> \~xs ->
        ((\[] -> false)
          | (\(~h : ~t) ->
                (((\true -> true)
                  | (\false -> (~rec ~t)))
                 ((~~eq) ~h ~k))))
        ~xs))
    ((~~nsMembers) ~ns)
  );

  # nsGetOr: 見つかれば (~ns ~k)、なければデフォルト ~d
  .nsGetOr = \~ns -> \~k -> \~d -> (
    ((~~fix)
      (\~rec -> \~xs ->
        ((\[] -> ~d)
          | (\(~h : ~t) ->
                (((\true  -> (~ns ~k))
                  | (\false -> (~rec ~t)))
                 ((~~eq) ~h ~k))))
        ~xs))
    ((~~nsMembers) ~ns)
  );
};
