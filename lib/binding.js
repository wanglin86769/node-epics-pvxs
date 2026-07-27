const fs = require('fs');
const path = require('path');

/** Supported prebuilds/<platform>-<arch>/ tags. Add entries when shipping new arches. */
const SUPPORTED_PREBUILDS = new Set(['win32-x64', 'linux-x64', 'darwin-x64']);

/** Windows: add prebuilds dir so LoadLibrary finds bundled DLLs beside pvxs.node. */
function configureWindowsDllPath(runtimeDir) {
  const delimiter = path.delimiter;
  if (process.env.PATH) {
    if (!process.env.PATH.split(delimiter).includes(runtimeDir)) {
      process.env.PATH = `${runtimeDir}${delimiter}${process.env.PATH}`;
    }
  } else {
    process.env.PATH = runtimeDir;
  }
  if (typeof process.addDllDirectory === 'function') {
    try {
      process.addDllDirectory(runtimeDir);
    } catch (_) {
      // ignore duplicate registration
    }
  }
}

/**
 * Load native addon from prebuilds/<platform>-<arch>/pvxs.node only.
 * After `npm run build`, copy pvxs.node (and libs) into that directory.
 * Linux/macOS shared libs: $ORIGIN / @loader_path (see scripts/set-*).
 */
function loadBinding() {
  const platformArch = `${process.platform}-${process.arch}`;
  if (!SUPPORTED_PREBUILDS.has(platformArch)) {
    throw new Error(
      `node-epics-pvxs: unsupported platform ${platformArch}. ` +
        `Supported: ${[...SUPPORTED_PREBUILDS].join(', ')}.`
    );
  }

  const runtimeDir = path.join(__dirname, '..', 'prebuilds', platformArch);
  const nodePath = path.join(runtimeDir, 'pvxs.node');
  if (!fs.existsSync(nodePath)) {
    throw new Error(
      `node-epics-pvxs: missing ${nodePath}. ` +
        'Copy build/Release/pvxs.node (and shared libs) into this prebuilds directory.'
    );
  }

  if (process.platform === 'win32') {
    configureWindowsDllPath(runtimeDir);
  }

  return require(nodePath);
}

module.exports = {
  loadBinding,
};
