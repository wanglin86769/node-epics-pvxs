/**
 * Client get (single + batch). Server.fromEnv peer required.
 *
 * Terminal A:  node examples/server-type-scalar.js
 * Terminal B:  node examples/client-get.js
 *
 * Optional:  $env:PV_NAME = "nodepvxs:ex:t:int"
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = process.env.PV_NAME || 'nodepvxs:ex:t:int';
const PV_B = 'nodepvxs:ex:t:str';

async function main() {
  const client = pvxs.client.Context.fromEnv();
  try {
    const one = await client.get(PV);
    console.log('get', PV, one.get('value').toString());

    const batch = await client.get([PV, PV_B]);
    console.log('batch', batch.map((v) => v.get('value').toString()));
  } finally {
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
