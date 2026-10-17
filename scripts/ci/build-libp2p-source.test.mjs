import assert from 'node:assert/strict';
import {test} from 'node:test';
import {spawnSync} from 'node:child_process';
import {mkdtempSync, mkdirSync, readFileSync, writeFileSync, chmodSync, symlinkSync, existsSync, statSync, rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';

// Uses only a tiny owned Git repository and a recording fake CMake; no compiler,
// remote clone, SDK, dependency cache or existing checkout is opened by the tests.
const pinnedCommit = 'c03bf7c44c4fd30a390f3ef93bd1eab482e686a6';
const scriptFile = process.env.NANO_LIBP2P_TEST_SCRIPT ?? fileURLToPath(new URL('../build-libp2p.sh', import.meta.url));
const scriptBytes = readFileSync(scriptFile, 'utf8');
const gitPath = spawnSync('/bin/sh', ['-c', 'command -v git'], {encoding: 'utf8'}).stdout.trim();
assert.ok(gitPath.startsWith('/'), 'an absolute test Git executable must be available');
const cleanEnv = {...process.env, GIT_CONFIG_NOSYSTEM: '1', GIT_CONFIG_GLOBAL: '/dev/null',
  GIT_CONFIG_SYSTEM: '/dev/null', GIT_AUTHOR_NAME: 'Synthetic source guard test',
  GIT_COMMITTER_NAME: 'Synthetic source guard test', GIT_AUTHOR_EMAIL: 'fixture@invalid',
  GIT_COMMITTER_EMAIL: 'fixture@invalid', GIT_TERMINAL_PROMPT: '0'};
for (const key of ['GIT_DIR', 'GIT_WORK_TREE', 'GIT_INDEX_FILE', 'GIT_CONFIG_COUNT', 'GIT_OBJECT_DIRECTORY', 'GIT_ALTERNATE_OBJECT_DIRECTORIES']) delete cleanEnv[key];

function invoke(command, args, options = {}) {
  const result = spawnSync(command, args, {encoding: 'utf8', timeout: 10000, maxBuffer: 1024 * 1024, env: cleanEnv, ...options});
  assert.ifError(result.error);
  return result;
}
function checked(command, args, options) {
  const result = invoke(command, args, options);
  assert.equal(result.status, 0, result.stderr || result.stdout);
  return result.stdout.trim();
}
function material(file) {
  const s = statSync(file, {bigint: true});
  return {nine: ['dev', 'ino', 'mode', 'nlink', 'uid', 'gid', 'size', 'mtimeNs', 'ctimeNs'].map(k => s[k].toString()),
    sha256: createHash('sha256').update(readFileSync(file)).digest('hex')};
}
function fixture(body, {fresh = false} = {}) {
  const temporary = mkdtempSync(join(tmpdir(), 'nano-libp2p-source-test-'));
  const root = join(temporary, 'repo');
  const deps = join(temporary, 'deps');
  const source = join(deps, 'cpp-libp2p');
  const seed = fresh ? join(temporary, 'seed') : source;
  const hooks = join(temporary, 'empty-hooks');
  const bin = join(temporary, 'bin');
  const trace = join(temporary, 'cmake.jsonl');
  const cloneTrace = join(temporary, 'clone.json');
  for (const dir of [join(root, 'scripts'), join(root, 'cmake'), deps, seed, hooks, bin]) mkdirSync(dir, {recursive: true});
  const git = (...args) => checked(gitPath, ['-c', `core.hooksPath=${hooks}`, '-c', 'commit.gpgsign=false', '-C', seed, ...args]);
  git('init', '-q');
  writeFileSync(join(seed, 'source.cpp'), 'int public_fixture() { return 7; }\n');
  writeFileSync(join(seed, '.gitignore'), 'ignored-input.hpp\n');
  writeFileSync(join(seed, 'CMakeLists.txt'), 'project(SyntheticOnly)\n');
  symlinkSync('source.cpp', join(seed, 'source-alias.cpp'));
  git('add', '.');
  git('commit', '-qm', 'synthetic pinned source');
  git('tag', 'v0.1.37');
  const commit = git('rev-parse', 'HEAD');
  assert.match(commit, /^[0-9a-f]{40}$/);
  assert.equal(scriptBytes.split(pinnedCommit).length, 2, 'test binds exactly the production pin, without replacing its guard');
  const script = join(root, 'scripts', 'build-libp2p.sh');
  writeFileSync(script, scriptBytes.replace(pinnedCommit, commit));
  writeFileSync(join(root, 'cmake', 'libp2p-toolchain.cmake'), '# synthetic unused toolchain\n');
  const cmake = join(bin, 'record-cmake');
  writeFileSync(cmake, `#!${process.execPath}
const fs = require('node:fs'); const cp = require('node:child_process');
const args = process.argv.slice(2);
const phase = args[0] === '--build' ? 'build' : args[0] === '--install' ? 'install' : 'configure';
fs.appendFileSync(process.env.TEST_TRACE, JSON.stringify({phase, args, hunterRoot: process.env.HUNTER_ROOT}) + '\\n');
if (phase === process.env.TEST_MUTATE_PHASE) {
  if (process.env.TEST_MUTATION === 'head') {
    const r = cp.spawnSync(process.env.REAL_GIT, ['-c', 'core.hooksPath=' + process.env.TEST_HOOKS, '-c', 'commit.gpgsign=false', '-C', process.env.TEST_SOURCE, 'commit', '--allow-empty', '-qm', 'synthetic changed HEAD'], {env: process.env});
    if (r.status !== 0) process.exit(91);
  } else fs.writeFileSync(process.env.TEST_SOURCE + '/source.cpp', 'changed during ' + phase + '\\n');
}
if (phase === process.env.TEST_FAIL_PHASE) { process.stderr.write('synthetic cmake failure\\n'); process.exit(42); }
`);
  chmodSync(cmake, 0o755);
  // Only intercept the fresh-clone branch; every guard read uses real Git.
  const gitWrapper = join(bin, 'git');
  writeFileSync(gitWrapper, `#!${process.execPath}
const cp = require('node:child_process'); const fs = require('node:fs'); const args = process.argv.slice(2);
const fresh = args[0] === 'clone';
if (fresh) fs.writeFileSync(process.env.TEST_CLONE_TRACE, JSON.stringify(args));
const actual = fresh ? ['clone', '--no-local', process.env.TEST_SEED, args.at(-1)] : args;
const r = cp.spawnSync(process.env.REAL_GIT, actual, {env: process.env, stdio: 'inherit'});
if (r.error) throw r.error; process.exit(r.status ?? 92);
`);
  chmodSync(gitWrapper, 0o755);
  const sentinels = ['libp2p-build', 'install', 'hunter'].map(dir => {
    mkdirSync(join(deps, dir), {recursive: true});
    const file = join(deps, dir, 'pre-existing-sentinel');
    writeFileSync(file, `preserve ${dir}\n`);
    return [file, material(file)];
  });
  const run = (extra = {}) => invoke('/bin/sh', [script], {env: {...cleanEnv, PATH: `${bin}:${cleanEnv.PATH ?? ''}`,
    NANO_DEPS_DIR: deps, CMAKE_BIN: cmake, REAL_GIT: gitPath, TEST_TRACE: trace, TEST_SOURCE: source,
    TEST_SEED: seed, TEST_HOOKS: hooks, TEST_CLONE_TRACE: cloneTrace, ...extra}});
  const calls = () => existsSync(trace) ? readFileSync(trace, 'utf8').trim().split('\n').map(JSON.parse) : [];
  try { body({root, deps, source, seed, commit, git, run, calls, cloneTrace}); }
  finally {
    try { for (const [file, before] of sentinels) assert.deepEqual(material(file), before, 'existing build/install/Hunter bytes and identity must remain intact'); }
    finally { rmSync(temporary, {recursive: true, force: true}); } // only this synthetic fixture
  }
}
function refusedBeforeCMake(f, result) {
  assert.notEqual(result.status, 0, 'dirty pinned source must refuse');
  assert.match(result.stderr, /cpp-libp2p source guard:/);
  assert.deepEqual(f.calls(), [], 'no configure/build/install may run on dirty source');
}

test('rejects an edited tracked source before CMake even with matching HEAD', () => {
  fixture(f => {
    writeFileSync(join(f.source, 'source.cpp'), 'dirty tracked bytes\n');
    assert.equal(f.git('rev-parse', 'HEAD'), f.commit);
    refusedBeforeCMake(f, f.run());
    assert.equal(readFileSync(join(f.source, 'source.cpp'), 'utf8'), 'dirty tracked bytes\n');
  });
});
test('clean pinned bytes preserve both source-only modes and all reserved sibling outputs', () => {
  for (const mode of ['0', '1']) fixture(f => {
    const result = f.run({NANO_LIBP2P_SOURCE_ONLY: mode});
    assert.equal(result.status, 0, result.stderr);
    const calls = f.calls();
    assert.deepEqual(calls.map(x => x.phase), ['configure', 'build', 'install']);
    assert.equal(calls[0].hunterRoot, join(f.deps, 'hunter'));
    for (const flag of ['-DHUNTER_USE_CACHE_SERVERS=NO', '-DHUNTER_KEEP_PACKAGE_SOURCES=ON', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
      assert.equal(calls[0].args.includes(flag), mode === '1');
    assert.ok(calls[0].args.includes('-DHUNTER_RUN_UPLOAD=OFF'));
    assert.equal(f.git('rev-parse', 'HEAD'), f.commit);
  });
});
test('a fresh checkout retains the pinned clone contract without network access in this test', () => {
  fixture(f => {
    const result = f.run({NANO_LIBP2P_SOURCE_ONLY: '1'});
    assert.equal(result.status, 0, result.stderr);
    assert.deepEqual(JSON.parse(readFileSync(f.cloneTrace, 'utf8')), ['clone', '--depth', '1', '--branch', 'v0.1.37', 'https://github.com/libp2p/cpp-libp2p.git', f.source]);
    assert.deepEqual(f.calls().map(x => x.phase), ['configure', 'build', 'install']);
  }, {fresh: true});
});
test('staged edits and index flags cannot hide changed source bytes from the guard', () => {
  for (const mode of ['staged', 'assume-unchanged', 'skip-worktree']) fixture(f => {
    if (mode !== 'staged') f.git('update-index', `--${mode}`, 'source.cpp');
    writeFileSync(join(f.source, 'source.cpp'), `dirty ${mode}\n`);
    if (mode === 'staged') f.git('add', 'source.cpp');
    refusedBeforeCMake(f, f.run());
    assert.equal(readFileSync(join(f.source, 'source.cpp'), 'utf8'), `dirty ${mode}\n`);
  });
});
test('untracked and ignored compiler inputs refuse before CMake and are never deleted', () => {
  for (const file of ['untracked-input.hpp', 'ignored-input.hpp']) fixture(f => {
    writeFileSync(join(f.source, file), 'must not be compiled or removed\n');
    refusedBeforeCMake(f, f.run());
    assert.equal(readFileSync(join(f.source, file), 'utf8'), 'must not be compiled or removed\n');
  });
});
test('source mutation during each CMake stage refuses at its boundary without cleanup or later stages', () => {
  for (const phase of ['configure', 'build', 'install']) fixture(f => {
    const result = f.run({TEST_MUTATE_PHASE: phase});
    assert.notEqual(result.status, 0, `${phase} mutation must refuse`);
    assert.match(result.stderr, /source bytes differ from pinned commit: source.cpp/);
    assert.deepEqual(f.calls().map(x => x.phase), ['configure', 'build', 'install'].slice(0, ['configure', 'build', 'install'].indexOf(phase) + 1));
    assert.equal(readFileSync(join(f.source, 'source.cpp'), 'utf8'), `changed during ${phase}\n`);
  });
});
test('HEAD changes refuse and the final fence preserves an earlier CMake failure code', () => {
  fixture(f => {
    const result = f.run({TEST_MUTATE_PHASE: 'configure', TEST_MUTATION: 'head'});
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /HEAD differs from dependencies.lock.json/);
    assert.notEqual(f.git('rev-parse', 'HEAD'), f.commit, 'guard must not reset source HEAD');
    assert.deepEqual(f.calls().map(x => x.phase), ['configure']);
  });
  fixture(f => {
    const result = f.run({TEST_MUTATE_PHASE: 'configure', TEST_FAIL_PHASE: 'configure'});
    assert.equal(result.status, 42, 'first child failure must survive the failing final fence');
    assert.match(result.stderr, /synthetic cmake failure/);
    assert.match(result.stderr, /source bytes differ from pinned commit: source.cpp/);
    assert.deepEqual(f.calls().map(x => x.phase), ['configure']);
  });
});
test('untracked and ignored empty directories refuse before CMake without deletion', () => {
  for (const directory of ['untracked-empty-directory', 'ignored-empty-directory']) fixture(f => {
    mkdirSync(join(f.source, directory));
    if (directory.startsWith('ignored-')) {
      writeFileSync(join(f.source, '.git', 'info', 'exclude'), 'ignored-empty-directory/\n');
      f.git('check-ignore', '--quiet', directory + '/');
    }
    refusedBeforeCMake(f, f.run());
    assert.equal(existsSync(join(f.source, directory)), true, 'guard must not remove an unknown directory');
  });
});
