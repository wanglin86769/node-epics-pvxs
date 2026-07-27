#!/usr/bin/env node

/**
 * Read config/epics-paths.js and resolve EPICS/pvxs paths for local builds.
 * Used by binding.gyp (CLI) for local builds.
 */

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const CONFIG_FILE = path.join(ROOT, 'config', 'epics-paths.js');

const REQUIRED = ['pvxsRoot', 'epicsBase', 'epicsHostArch'];

const CLI_KEYS = {
  pvxs_root: 'pvxsRoot',
  epics_base: 'epicsBase',
  pvxs_lib_dir: 'pvxsLibDir',
  epics_base_lib_dir: 'epicsLibDir',
  libevent_lib_dir: 'libeventLibDir',
};

function loadConfig() {
  if (!fs.existsSync(CONFIG_FILE)) {
    throw new Error(
      `Missing ${CONFIG_FILE}. Copy config/epics-paths.example.js to config/epics-paths.js and set your paths.`
    );
  }

  const cfg = require(CONFIG_FILE);

  if (!cfg || typeof cfg !== 'object' || Array.isArray(cfg)) {
    throw new Error('config/epics-paths.js must module.exports a plain object');
  }

  for (const key of REQUIRED) {
    if (!cfg[key] || typeof cfg[key] !== 'string') {
      throw new Error(`config/epics-paths.js must export string "${key}"`);
    }
  }

  return cfg;
}

function resolvePaths() {
  const cfg = loadConfig();
  const pvxsRoot = path.resolve(cfg.pvxsRoot);
  const epicsBase = path.resolve(cfg.epicsBase);
  const arch = cfg.epicsHostArch;

  return {
    pvxsRoot,
    epicsBase,
    epicsHostArch: arch,
    pvxsLibDir: path.join(pvxsRoot, 'lib', arch),
    epicsLibDir: path.join(epicsBase, 'lib', arch),
    libeventLibDir: path.join(pvxsRoot, 'bundle', 'usr', arch, 'lib'),
  };
}

if (require.main === module) {
  const key = process.argv[2];
  if (!key || !(key in CLI_KEYS)) {
    process.stderr.write(
      `Usage: node scripts/resolve-epics-paths.js <${Object.keys(CLI_KEYS).join('|')}>\n`
    );
    process.exit(1);
  }
  try {
    const paths = resolvePaths();
    process.stdout.write(paths[CLI_KEYS[key]].replace(/\\/g, '/'));
  } catch (err) {
    process.stderr.write(`${err.message}\n`);
    process.exit(1);
  }
}

module.exports = { resolvePaths };
