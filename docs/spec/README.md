Language Specification Policy

Authoritative source
- The language behavior is defined by documents in this folder (docs/spec/) and the BNF in docs/parser-bnf.md.
- The implementation (lexer/parser/runtime) must conform to these documents. Implementation changes do not constitute the spec.

Change control
- Any change to src/parser/lexer.l or src/parser/parser.y that alters tokens, precedence, associativity, or grammar rules must be accompanied by an update to docs/parser-bnf.md and/or docs/spec/*.
- Spec document changes require repository-owner approval and a signed-off commit: add a footer line `Signed-off-by: <owner> <email>`.

Automation
- CI workflow .github/workflows/spec-guard.yml blocks grammar changes without corresponding spec updates.
- CODEOWNERS enforces owner review on spec and grammar files.

Developer workflow
- Propose grammar/spec changes in a PR that first updates docs/spec/* and docs/parser-bnf.md. Include rationale and examples.
- Only after approval update implementation to match the spec.
- If an emergency implementation fix is needed, set env ALLOW_SPEC_CHANGE_WITHOUT_DOCS=1 in the workflow dispatch and open a follow-up PR to update the spec.

Scope
- This policy covers: tokens, literals, operators (new or precedence changes), do-block syntax, namespace literal forms, effect sugar (!, ~~), and any semantic changes user-visible in the language.
