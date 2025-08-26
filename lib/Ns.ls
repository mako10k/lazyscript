{
  .nsMembers = (~~nsMembers);

  # nsHas: 直接ルックアップで #err 判定（値は評価しない）
  #   (~ns ~k) が undefined なら #err を返す仕様を利用
  .nsHas = \~ns -> \~k -> (
    (((\#err ~_ -> false)
      || (\_        -> true))
     ((~ns ~k)))
  );

  # nsGetOr: 見つかれば値（未評価のまま）、なければデフォルト ~d
  .nsGetOr = \~ns -> \~k -> \~d -> (
    (((\#err ~_ -> ~d)
      || (\~v       -> ~v))
     ((~ns ~k)))
  );
};
