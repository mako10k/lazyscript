# Test/init-time binding for the value-namespace Prelude
# Bind ~Prelude via effectful def so tests can use (~Prelude .to_str) etc.
!{
	# Core のビルトイン名前空間と internal API を事前に公開
	!def builtins ((~prelude .builtin) "core");
	!def internal  (~prelude .env);

		# 互換性のため、主要ビルトイン（add/lt/eq/print 等）をトップレベルにも読み込む
		((~prelude .env .import) ~builtins);

	# Prelude 値を include で読み込む（Prelude.ls 内で ~builtins/~internal を参照可能）
	!def Prelude ((~prelude .env .include) "lib/Prelude.ls");
};
