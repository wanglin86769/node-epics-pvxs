/**
 * onPut validation: reject negative values with op.error (Server.fromEnv).
 *
 * Run:  node examples/server-put-error.js
 * Stop: Ctrl+C
 *
 * While running:
 *   pvput nodepvxs:ex:ge0 5     # accepted
 *   pvput nodepvxs:ex:ge0 -1    # rejected: value must be >= 0
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:ge0';

function withTimeStamp(fields) {
  const unixSec = Date.now() / 1000;
  const sec = Math.floor(unixSec);
  const ns = Math.floor((unixSec - sec) * 1e9);
  return Object.assign({}, fields, {
    timeStamp: { secondsPastEpoch: sec, nanoseconds: ns, userTag: 0 },
  });
}

async function main() {
  const pv = SharedPV.buildMailbox();
  pv.open(NTScalar.create(TypeCode.Int32, withTimeStamp({ value: 0 })));
  pv.onPut((p, op) => {
    const n = op.value().get('value').asInt();
    if (n < 0) {
      op.error('value must be >= 0');
      return;
    }
    p.post(withTimeStamp({ value: n }));
    op.reply();
  });

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving ${PV} (rejects value < 0). Ctrl+C to stop.`);
  console.log(`  pvput ${PV} 5`);
  console.log(`  pvput ${PV} -1`);

  const shutdown = () => {
    server.stop();
    pv.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
