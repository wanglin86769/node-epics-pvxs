/**
 * Client rpc. Server.fromEnv peer required.
 *
 * Terminal A:  node examples/server-rpc.js
 * Terminal B:  node examples/client-rpc.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = 'nodepvxs:ex:echo';

async function main() {
  const client = pvxs.client.Context.fromEnv();
  try {
    const reply = await client.rpc(PV, { msg: 'hello', n: 3 });
    console.log(JSON.stringify(reply.toObject(), null, 2));
  } finally {
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
