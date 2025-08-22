# should resolve ~'foo' as ref to name foo
!{ ~~def foo 3; ((~prelude println) ((~to_str) (~'foo'))); };
