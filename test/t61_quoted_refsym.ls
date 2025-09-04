# should resolve ~'foo' as ref to name foo
!{
	((~prelude .env .import) { .foo = 3 });
	!println (~~to_str (~'foo'))
};
