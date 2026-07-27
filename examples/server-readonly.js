/**
 * Readonly SharedPV (Server.fromEnv).
 *
 * Run:  node examples/server-readonly.js
 * Stop: Ctrl+C
 *
 * While running:
 *   pvget nodepvxs:ex:readonly      # ok
 *   pvput nodepvxs:ex:readonly 1    # rejected (read-only)
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:readonly';

function withTimeStamp(fields) {
  const unixSec = Date.now() / 1000;
  const sec = Math.floor(unixSec);
  const ns = Math.floor((unixSec - sec) * 1e9);
  return Object.assign({}, fields, {
    timeStamp: { secondsPastEpoch: sec, nanoseconds: ns, userTag: 0 },
  });
}

async function main() {
  const pv = SharedPV.buildReadonly();
  pv.open(NTScalar.create(TypeCode.Int32, withTimeStamp({ value: 42 })));

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving ${PV} = 42 (readonly). Ctrl+C to stop.`);
  console.log(`  pvget ${PV}`);
  console.log(`  pvput ${PV} 1`);

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
