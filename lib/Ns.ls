
{
  // Closure (expr; pat = expr; ...) を let rec 的に扱う前提で、
  // 名前空間のメンバーリスト取得
  .nsMembers = ~~nsMembers;

  // メンバー存在判定: nsHas(ns, m)
  .nsHas ~ns ~m = (
    // nsMembers から m が含まれているか
    ~~any (\x -> ~~eq x ~m) (~~nsMembers ~ns)
  );

  // メンバー取得（なければデフォルト）: nsGetOr(ns, m, d)
  .nsGetOr ~ns ~m ~d = (
    ~~if (~~nsHas ~ns ~m) (~ns ~m) ~d
  );
};