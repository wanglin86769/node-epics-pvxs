'use strict';

const assert = require('assert');
const pvxs = require('..');

function main() {
  const ver = pvxs.version();
  assert.strictEqual(typeof ver, 'string');
  assert.ok(ver.length > 0, 'version string should not be empty');

  const verInt = pvxs.versionInt();
  assert.strictEqual(typeof verInt, 'number');
  assert.ok(verInt > 0, 'versionInt should be positive');

  const verAbi = pvxs.versionAbi();
  assert.strictEqual(typeof verAbi, 'number');
  assert.ok(verAbi > 0, 'versionAbi should be positive');

  console.log('version.test.js: ok');
}

try {
  main();
} catch (err) {
  console.error(err);
  process.exitCode = 1;
}
