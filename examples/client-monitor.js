/**
 * Client monitor (pull / for-await). Runs until Ctrl+C.
 *
 * Terminal A:  node examples/server-post.js
 * Terminal B:  node examples/client-monitor.js
 *
 * First event is Disconnected (subscription starts unconnected); then Values.
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = 'nodepvxs:ex:post';

async function main() {
  const client = pvxs.client.Context.fromEnv();
  const { Disconnected } = pvxs.client;
  const monitor = client.monitor(PV);

  const shutdown = () => {
    monitor.cancel();
    client.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);

  console.log(`monitor ${PV} (Ctrl+C to stop)...`);
  let n = 0;
  for await (const ev of monitor) {
    if (ev instanceof Disconnected) {
      console.log('[Disconnected]');
      continue;
    }
    n += 1;
    console.log(`#${n}`, ev.get('value').toString());
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
