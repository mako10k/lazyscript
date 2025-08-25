const vscode = require('vscode');
const cp = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

function runCmd(cmd, args, options, inputText) {
  return new Promise((resolve) => {
    const child = cp.spawn(cmd, args, { ...options, shell: false });
    let stdout = '';
    let stderr = '';
    if (inputText != null) {
      child.stdin.write(inputText);
      child.stdin.end();
    }
    child.stdout.on('data', (d) => (stdout += d.toString()));
    child.stderr.on('data', (d) => (stderr += d.toString()));
    child.on('error', (err) => resolve({ code: -1, stdout, stderr: String(err) }));
    child.on('close', (code) => resolve({ code, stdout, stderr }));
  });
}

function createTmpWith(text, ext = '.ls') {
  const file = path.join(os.tmpdir(), `lazyscript-vscode-${Date.now()}-${Math.random().toString(36).slice(2)}${ext}`);
  fs.writeFileSync(file, text, 'utf8');
  return file;
}

function parseDiagnostics(output, docUri) {
  // Example error: "<file>:1.19-21: syntax error, unexpected LSTDOTSYMBOL, expecting LSTSYMBOL"
  const diags = [];
  const re = /^(.*?):(\d+)\.(\d+)(?:-\d+)?\:\s+(.*)$/gm;
  let m;
  while ((m = re.exec(output)) !== null) {
    const file = m[1];
    const line = Math.max(0, parseInt(m[2], 10) - 1);
    const col = Math.max(0, parseInt(m[3], 10) - 1);
    const message = m[4];
    try {
      if (docUri.fsPath && file && path.resolve(file) !== path.resolve(docUri.fsPath)) {
        // Only surface diagnostics for the active document
        continue;
      }
    } catch (_) {}
    const range = new vscode.Range(line, col, line, col + 1);
    const diag = new vscode.Diagnostic(range, message, vscode.DiagnosticSeverity.Error);
    diags.push(diag);
  }
  return diags;
}

function resolveTool(cfgKey, defaultName) {
  const cfg = vscode.workspace.getConfiguration('lazyscript');
  let tool = cfg.get(cfgKey, defaultName);
  // If it's just a bare name, try repo-local ./src/<name>
  if (!tool.includes(path.sep)) {
    const folders = vscode.workspace.workspaceFolders || [];
    for (const f of folders) {
      const candidate = path.join(f.uri.fsPath, 'src', tool);
      try {
        if (fs.existsSync(candidate)) {
          tool = candidate;
          break;
        }
      } catch (_) {}
    }
  }
  return tool;
}

async function provideFormatting(document, _options, token) {
  const formatter = resolveTool('formatterPath', 'lazyscript_format');
  // Use temp file to be robust regardless of formatter stdin support
  const tmpFile = createTmpWith(document.getText(), '.ls');
  try {
    const { code, stdout, stderr } = await runCmd(formatter, [tmpFile], { cwd: path.dirname(document.uri.fsPath) });
    if (token.isCancellationRequested) return [];
    if (code !== 0) {
      const msg = stderr || stdout || `Formatter exited with code ${code}`;
      vscode.window.showWarningMessage(`lazyscript_format: ${msg.trim()}`);
      return [];
    }
    const edits = [vscode.TextEdit.replace(new vscode.Range(0, 0, document.lineCount, 0), stdout.replace(/\r\n/g, '\n'))];
    return edits;
  } finally {
    try { fs.unlinkSync(tmpFile); } catch (_) {}
  }
}

function activate(context) {
  const collection = vscode.languages.createDiagnosticCollection('lazyscript');
  context.subscriptions.push(collection);

  const validate = async (doc) => {
    if (!doc || doc.languageId !== 'lazyscript') return;
  const lazyscript = resolveTool('lazyscriptPath', 'lazyscript');
    const tmpFile = createTmpWith(doc.getText(), '.ls');
    const opts = { cwd: path.dirname(doc.uri.fsPath) };
    try {
      const { stdout, stderr } = await runCmd(lazyscript, [tmpFile], opts);
      const out = `${stdout}\n${stderr}`;
      const diags = parseDiagnostics(out, doc.uri);
      collection.set(doc.uri, diags);
    } finally {
      try { fs.unlinkSync(tmpFile); } catch (_) {}
    }
  };

  // Debounced validation on change
  const timers = new Map();
  const debounceMs = () => vscode.workspace.getConfiguration('lazyscript').get('diagnostics.debounceMs', 300);
  const scheduleValidate = (doc) => {
    const key = doc.uri.toString();
    clearTimeout(timers.get(key));
    timers.set(key, setTimeout(() => validate(doc), debounceMs()));
  };

  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument((doc) => doc.languageId === 'lazyscript' && scheduleValidate(doc)),
    vscode.workspace.onDidChangeTextDocument((e) => e.document.languageId === 'lazyscript' && scheduleValidate(e.document)),
    vscode.workspace.onDidSaveTextDocument((doc) => doc.languageId === 'lazyscript' && scheduleValidate(doc)),
    vscode.workspace.onDidCloseTextDocument((doc) => collection.delete(doc.uri))
  );

  // Register formatter
  context.subscriptions.push(
    vscode.languages.registerDocumentFormattingEditProvider({ language: 'lazyscript' }, { provideDocumentFormattingEdits: provideFormatting })
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
