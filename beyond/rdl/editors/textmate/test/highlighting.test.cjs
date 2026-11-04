const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { before, test } = require('node:test');
const { Registry, INITIAL } = require('vscode-textmate');
const oniguruma = require('vscode-oniguruma');

const root = path.resolve(__dirname, '..');
const manifest = JSON.parse(fs.readFileSync(path.join(root, 'package.json'), 'utf8'));
const contribution = manifest.contributes.grammars[0];
let grammar;

before(async () => {
  const wasm = fs.readFileSync(require.resolve('vscode-oniguruma/release/onig.wasm'));
  await oniguruma.loadWASM(wasm.buffer.slice(wasm.byteOffset, wasm.byteOffset + wasm.byteLength));
  const registry = new Registry({
    onigLib: Promise.resolve({
      createOnigScanner: patterns => new oniguruma.OnigScanner(patterns),
      createOnigString: text => new oniguruma.OnigString(text)
    }),
    loadGrammar: async scope => scope === contribution.scopeName
      ? JSON.parse(fs.readFileSync(path.join(root, contribution.path), 'utf8')) : null
  });
  grammar = await registry.loadGrammar(contribution.scopeName);
});

function tokenize(text) {
  const lines = text.split(/\r?\n/);
  let state = INITIAL;
  const tokens = [];
  for (let line = 0; line < lines.length; line++) {
    const result = grammar.tokenizeLine(lines[line], state);
    for (const token of result.tokens) {
      tokens.push({ line, start: token.startIndex, end: token.endIndex,
        text: lines[line].slice(token.startIndex, token.endIndex), scopes: token.scopes });
    }
    state = result.ruleStack;
  }
  return { tokens, state };
}

function scoped(result, text, scope) {
  assert(result.tokens.some(token => token.text === text && token.scopes.some(name => name.startsWith(scope))),
    `expected ${JSON.stringify(text)} in ${scope}; got ${JSON.stringify(result.tokens)}`);
}

test('type, trait, object, let, and derived declarations receive distinct scopes', () => {
  const result = tokenize('trait type primary = { name : string; }\ntrait hyper : primary = {}\nrace terrans = {}\nlet count = 1\nderive points : int = 1');
  scoped(result, 'primary', 'entity.name.type');
  scoped(result, 'hyper', 'entity.name.variable');
  scoped(result, 'race', 'entity.name.type');
  scoped(result, 'terrans', 'entity.name.variable');
  scoped(result, 'count', 'entity.name.variable');
  scoped(result, 'points', 'variable.other.property');
});

test('all reserved words are recognized without matching prefixes of identifiers', () => {
  const keywords = ['type','trait','object','let','in','if','then','else','match','with','of','derive','require',
    'query','from','where','select','order','by','ascending','descending','and','or','not','import'];
  for (const keyword of keywords) {
    const result = tokenize(keyword);
    assert(result.tokens.some(token => token.text === keyword && token.scopes.some(scope => /^(keyword|storage)\./.test(scope))), keyword);
  }
  const result = tokenize('selectable integer_value immune_system');
  for (const name of ['selectable','integer_value','immune_system']) scoped(result, name, 'variable.other');
});

test('compact ranges keep separate integer endpoints and a range operator', () => {
  const result = tokenize('20..80 0.5..2.0 -20 1e-3 2E+4');
  for (const value of ['20','80','0.5','2.0','1e-3','2E+4']) scoped(result, value, 'constant.numeric');
  assert.equal(result.tokens.filter(token => token.text === '..' && token.scopes.includes('keyword.operator.range.rdl')).length, 2);
});

test('imports color dotted module names as namespaces', () => {
  const result = tokenize('import stars.traits;\nimport schema');
  scoped(result, 'import', 'keyword.control.import');
  scoped(result, 'stars.traits', 'entity.name.namespace');
  scoped(result, 'schema', 'entity.name.namespace');
});

test('normal strings support escapes and flag invalid escapes', () => {
  const result = tokenize(String.raw`let text = "where // (* \n \" quoted \q end";`);
  scoped(result, '\\n', 'constant.character.escape');
  scoped(result, '\\"', 'constant.character.escape');
  scoped(result, '\\q', 'invalid.illegal.escape');
  assert(!result.tokens.some(token => token.scopes.some(scope => scope.startsWith('comment.'))));
});

test('raw multiline strings preserve quotes, backslashes, and language-like text', () => {
  const result = tokenize('let text = """\nwhere // (* "quoted" \\n\n🌍 星\n""";\nlet x = 1');
  assert(result.tokens.some(token => token.text.includes('where //') && token.scopes.includes('string.quoted.triple.rdl')));
  assert(!result.tokens.some(token => token.scopes.some(scope => /^(comment\.|constant\.character\.escape)/.test(scope))));
  scoped(result, 'x', 'entity.name.variable');
  assert.equal(result.state.depth, 1);
});

test('nested block comments survive line changes and close at the correct depth', () => {
  const result = tokenize('(* outer\n (* inner *) still outer\n*) let x = 1');
  assert(result.tokens.some(token => token.scopes.filter(scope => scope === 'comment.block.rdl').length === 2));
  assert(result.tokens.some(token => token.text.includes('still outer') && token.scopes.includes('comment.block.rdl')));
  scoped(result, 'x', 'entity.name.variable');
});

test('line comments reset at newline and ignore embedded string delimiters', () => {
  const result = tokenize('// """ where\nlet x = 1');
  assert(result.tokens.some(token => token.text.includes('where') && token.scopes.includes('comment.line.double-slash.rdl')));
  scoped(result, 'x', 'entity.name.variable');
  assert.equal(result.state.depth, 1);
});

test('type variables, primitive types, literals, and builtin functions are colored', () => {
  const result = tokenize("'a int integer real string bool option set range immune none true false Set.contains List.sum round");
  scoped(result, "'a", 'entity.name.type.parameter');
  for (const type of ['int','integer','real','string','bool','option','set','range']) scoped(result, type, 'support.type');
  for (const value of ['immune','none','true','false']) scoped(result, value, 'constant.language');
  for (const name of ['contains','sum','round']) scoped(result, name, 'support.function');
});

test('field labels and member accesses receive property scopes', () => {
  const result = tokenize('population = 25000; gravity : real; self.population + r.points');
  scoped(result, 'population', 'variable.other.property');
  scoped(result, 'gravity', 'variable.other.property');
  scoped(result, 'points', 'variable.other.property');
});

test('unfinished ordinary strings stop at the line; multiline constructs remain open', () => {
  scoped(tokenize('let text = "unfinished\nlet next = 1'), 'next', 'entity.name.variable');
  for (const [text, scope] of [['""" unfinished\nlet next = 1', 'string.quoted.triple.rdl'], ['(* unfinished\nlet next = 1', 'comment.block.rdl']]) {
    const result = tokenize(text);
    assert(result.tokens.filter(token => token.line === 1).every(token => token.scopes.includes(scope)));
    assert(result.state.depth > 1);
  }
});

test('the shipped race example finishes in normal code with no invalid escape scopes', () => {
  const result = tokenize(fs.readFileSync(path.join(root, '../../examples/races.rdl'), 'utf8'));
  assert.equal(result.state.depth, 1);
  assert(!result.tokens.some(token => token.scopes.some(scope => scope.startsWith('invalid.'))));
  scoped(result, 'terrans', 'entity.name.variable');
  scoped(result, 'habitability_points', 'variable.other.property');
});
