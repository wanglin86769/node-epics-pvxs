/**
 * Client list(server). Lists PV names from one PVA peer.
 *
 * `server` must be an IP (optional :port), e.g. 127.0.0.1 or 192.168.1.38:5075.
 * Prefer the `server=` string from client-discover.js. Avoid bare "localhost"
 * (hostname path can crash the native addon on Windows).
 *
 * Terminal A:  node examples/server-mailbox.js
 * Terminal B:  node examples/client-list.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const SERVER = '127.0.0.1';

async function main() {
  const client = pvxs.client.Context.fromEnv();
  try {
    console.log(`list(${SERVER})...`);
    const listing = await client.list(SERVER);
    // NTScalarArray: string[] value = [...]
    const field = listing.get('value');
    const arr =
      field && typeof field.asArray === 'function' ? field.asArray() : [];
    console.log(`${arr.length} name(s)`);
    if (arr.length) console.log(arr.join('\n'));
  } finally {
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});