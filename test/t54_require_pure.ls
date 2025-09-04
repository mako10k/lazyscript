!{
	# ピュアモジュールを require（副作用: ロード）。戻りは unit ()
	~r <- !require "test/pure_mod.ls";
	{- #include "test/pure_mod.ls" -}
	# エクスポートの確認（included モジュールの値はトップ名で参照）
	!println (~~to_str (Foo));
	!println (~~to_str ((Bar 41)));
	# require の戻り（unit）を最後に出力
	!println (~~to_str ~r);
};
