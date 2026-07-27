/**
 * onRPC echo: reply with the request value (Server.fromEnv).
 *
 * Run:  node examples/server-rpc.js
 * Stop: Ctrl+C
 *
 * While running (pvcall args must be name=value, not a bare string):
 *   pvcall nodepvxs:ex:echo msg=hello n=3
 *   pvcall nodepvxs:ex:echo aaa=1
 *
 * Or from this package: node examples/client-rpc.js
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:echo';

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
  pv.open(NTScalar.create(TypeCode.String, withTimeStamp({ value: 'ready' })));
  pv.onRPC((_p, op) => {
    op.reply(op.value());
  });

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving ${PV} (RPC echo). Ctrl+C to stop.`);
  console.log('Try (args must be name=value):');
  console.log(`  pvcall ${PV} msg=hello n=3`);
  console.log(`  pvcall ${PV} aaa=1`);

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
