/**
 * Client putMany. Server.fromEnv peer required.
 *
 * Reads current values, then putMany, then reads back.
 *
 * Terminal A:  node examples/server-type-scalar.js
 * Terminal B:  node examples/client-put-many.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');

async function main() {
  const client = pvxs.client.Context.fromEnv();
  try {
    const before = await client.get(['nodepvxs:ex:t:int', 'nodepvxs:ex:t:str']);
    console.log('before', before[0].get('value').asInt(), before[1].get('value').asString());

    // Each put also fetchPresent(true) so the native put merges against the live type/value.
    const results = await client.putMany({
      'nodepvxs:ex:t:int': { value: 10 },
      'nodepvxs:ex:t:str': { value: 'batch' },
    });
    console.log('putMany keys', Object.keys(results));

    const after = await client.get(['nodepvxs:ex:t:int', 'nodepvxs:ex:t:str']);
    console.log('after', after[0].get('value').asInt(), after[1].get('value').asString());
  } finally {
    client.close();
  }
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
