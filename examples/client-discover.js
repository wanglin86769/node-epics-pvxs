/**
 * Client discover (beacon / search peers).
 *
 * Prints each discovered peer until you stop (Ctrl+C).
 *
 * Run:  node examples/client-discover.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');

async function main() {
  const client = pvxs.client.Context.fromEnv();
  const { Finished } = pvxs.client;
  const handle = client.discover({ ping: true });

  const shutdown = () => {
    handle.cancel();
    client.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);

  try {
    console.log('discover running (Ctrl+C to stop)...');
    let n = 0;
    for await (const entry of handle) {
      if (entry instanceof Finished) break;
      if (typeof entry.peer === 'string') {
        n += 1;
        console.log(`#${n}`, 'peer=', entry.peer, 'server=', entry.server, 'proto=', entry.proto);
        continue;
      }
      const name =
        entry && entry.constructor && entry.constructor.name
          ? entry.constructor.name
          : typeof entry;
      console.log(`[event ${name}]`);
    }
  } finally {
    handle.cancel();
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
