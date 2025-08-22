!{
  # GC 再現の最小候補: ns literal の構築→即アクセス（'|' を含まない）
  ~ns <- { .A = { .B = 1 } };
  ~~println (~to_str (~ns.A.B));
};
