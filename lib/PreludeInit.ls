# Test/init-time binding for value-level prelude and conveniences without !def
!{
	# 互換レイヤは廃止: トップレベルへのコア内蔵関数の一括 import は行わない

	# 値としての builtins/internal/prelude を一括で現在環境に公開
	((~prelude .env .import)
		{
			.builtins = ((~prelude .builtin) "core");
			.internal = (~prelude .env);
			.prelude  = ((~prelude .env .include) "lib/Prelude.ls");
		}
	);
};
