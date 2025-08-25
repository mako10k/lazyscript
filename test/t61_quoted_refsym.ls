# should resolve ~'foo' as ref to name foo
!{ !def foo 3; !println (~~to_str (~'foo')) };
