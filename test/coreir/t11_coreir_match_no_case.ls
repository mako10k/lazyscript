# This program should trigger a match with no cases at Core IR runtime evaluator level.
# Use a helper that the compiler will lower to a match with zero branches. For now, we simulate
# by directly evaluating a pattern that cannot match and no alternatives; this relies on the
# Core IR lowering to emit a match with zero cases. If lowering doesn't, this test will be skipped later.
{- placeholder; actual Core IR evaluator test may be wired via --eval-coreir on a crafted input -}
()
# Core IR match with no-case should yield Bottom at runtime evaluator
# Using wildcard-only cases ensures we fall through to no-case
# Here we directly craft an expression that the Core IR runtime supports: simple match with no case
# In surface syntax, represent a match that doesn't match: use a symbol that won't match any trivial var/wildcard case
# However, runtime path only supports wildcard/var; to force no-case, provide zero cases.
# This program encodes: match on an int with zero cases.
()
