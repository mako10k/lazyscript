!{
	# ピュアモジュールを require（副作用: ロード）。戻りは unit ()
	~r <- !require "test/pure_mod.ls";
	# 値としてのモジュール本体は include で取得
	~M <- ~~include "test/pure_mod.ls";
	# エクスポートの確認
	!println (~~to_str ((~M .Foo)));
	!println (~~to_str (((~M .Bar) 41)));
	# require の戻り（unit）を最後に出力
	!println (~~to_str ~r);
};
