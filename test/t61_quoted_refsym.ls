# should resolve ~'foo' as ref to name foo
!{
	# 直接ローカルに foo を定義
	~foo = 3;
	!println (~~to_str (~'foo'))
};
