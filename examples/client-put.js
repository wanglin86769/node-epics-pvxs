/**
 * Client put. Server.fromEnv peer required.
 *
 * Terminal A:  node examples/server-mailbox.js
 * Terminal B:  node examples/client-put.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = 'nodepvxs:ex:mailbox';

async function main() {
  const client = pvxs.client.Context.fromEnv();
  try {
    const before = await client.get(PV);
    console.log('before', before.get('value').asInt());

    await client.put(PV, { value: 42 });
    const after = await client.get(PV);
    console.log('after', after.get('value').asInt());
  } finally {
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
