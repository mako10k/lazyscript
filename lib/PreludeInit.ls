# Test/init-time binding for value-level prelude (no legacy import/include)
!{
	# 互換レイヤは廃止: トップレベルへの一括 import は行わない
	# 必要な値を個別に束縛
	~core <- ((~prelude .builtin) "core");
	builtins = ~core;
	internal = (~prelude .env);
	{- #include "lib/Prelude.ls" -}
	prelude = Prelude;
};
