# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
- TBD

## [0.0.1-next] - 2025-08-20
### Added
- Symbol literal `.name` (interned); const keys support: symbol/string/int/0-arity constructor.
- Namespaces overhaul: named namespaces, anonymous namespace values, immutable literal namespaces.
- nsMembers: returns `list<const>` with stable ordering (symbol < string < int < constr).
- Strict-effects integration: guard ns mutations (nsnew/nsnew0/nsdef/nsdefv/__set); tokenized via seq/chain in pure contexts.
- Runtime immutability for literal namespaces: updates via nsdefv/__set now error.
- Tests: t39–t41 (strict-effects), t42–t44 (immutable literal/assigned), t45 (unknown namespace).

### Changed
- Error messages standardized: `...: const expected`, `nsdef: unknown namespace`, `nsMembers: not a namespace`.
- nsdef/nsdefv/set error surfaces unified to `#err` results; stderr used for pure-context effect violations.

### Docs
- docs/10_namespaces.md updated to reflect runtime immutability, ordering, and strict-effects.
- docs/12_builtins_and_prelude.md refined for current nsMembers behavior and `.symbol` status.
- docs/14_namespace_impl_plan.md annotated with implemented phases, remaining items, and exact error model.

