
{
  # 名前空間のメンバーリスト取得（純）
  .nsMembers = (~~nsMembers);

  # メンバー存在判定: nsHas(ns, m)
  .nsHas ~ns ~m = (
    ~go ~ms;
    ~go = (
      \[] -> false |
      \(~h : ~t) -> (((\true -> true) | (\false -> (~go ~t))) (((~~eq ~h) ~m)))
    );
    ~ms = ((~~nsMembers) ~ns)
  );

  # メンバー取得（なければデフォルト）: nsGetOr(ns, m, d)
  .nsGetOr ~ns ~m ~d = (
    ~go ~ms;
    ~go = (
      \[] -> ~d |
      \(~h : ~t) -> (((\true -> (~ns ~m)) | (\false -> (~go ~t))) (((~~eq ~h) ~m)))
    );
    ~ms = ((~~nsMembers) ~ns)
  );
};