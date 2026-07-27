/**
 * Client Config: fromEnv / fromDefs / fromConfig.
 *
 * Builds contexts three ways, then get() once (needs a peer PV).
 *
 * Terminal A:  node examples/server-mailbox.js
 * Terminal B:  node examples/client-config.js
 *
 * Optional:  $env:PV_NAME = "nodepvxs:ex:mailbox"
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = process.env.PV_NAME || 'nodepvxs:ex:mailbox';

async function main() {
  const { Config, Context } = pvxs.client;

  const c1 = Context.fromConfig(Config.fromEnv());
  try {
    console.log('fromEnv+fromConfig', (await c1.get(PV)).get('value').toString());
  } finally {
    c1.close();
  }

  const c2 = Context.fromConfig(
    Config.fromDefs({
      EPICS_PVA_AUTO_ADDR_LIST: 'YES',
    })
  );
  try {
    console.log('fromDefs', (await c2.get(PV)).get('value').toString());
  } finally {
    c2.close();
  }

  const c3 = Context.fromConfig({
    EPICS_PVA_AUTO_ADDR_LIST: true,
  });
  try {
    console.log('plain fromConfig', (await c3.get(PV)).get('value').toString());
  } finally {
    c3.close();
  }

  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
