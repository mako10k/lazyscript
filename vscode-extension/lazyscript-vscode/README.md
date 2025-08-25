# LazyScript VS Code Extension

Features:
- Syntax highlighting for `.ls`
- Diagnostics using `lazyscript` (parse errors inline)
- Formatting using `lazyscript_format`

## Quick start
1) Open this folder in VS Code: `vscode-extension/lazyscript-vscode`
2) Press F5 to launch an Extension Development Host.
3) Open a `.ls` file and start typing.

Alternatively, run VS Code with this extension in-place:

```
code --extensionDevelopmentPath /absolute/path/to/vscode-extension/lazyscript-vscode
```

## Settings
- `lazyscript.lazyscriptPath` (default: `lazyscript`)
- `lazyscript.formatterPath` (default: `lazyscript_format`)
- `lazyscript.diagnostics.debounceMs` (default: 300)

## How it works
- Diagnostics: runs the configured `lazyscript` binary on a temp file snapshot of the document and parses messages like `file:line.col-...: syntax error ...` into diagnostics.
- Formatting: runs `lazyscript_format` on a temp file containing the document and replaces the content with its stdout.

## Notes
- Ensure your `PATH` has the binaries or set absolute paths via settings.
- The grammar captures comments, strings, numbers, refs (`~name`), and `.member` symbols.
