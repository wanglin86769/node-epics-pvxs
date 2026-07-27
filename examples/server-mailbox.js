/**
 * Mailbox SharedPV serving a fixed value (Server.fromEnv).
 *
 * Run:  node examples/server-mailbox.js
 * Stop: Ctrl+C
 *
 * While running: pvget nodepvxs:ex:mailbox
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:mailbox';

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
  pv.open(NTScalar.create(TypeCode.Int32, withTimeStamp({ value: 100 })));
  pv.onPut((p, op) => {
    p.post(withTimeStamp({ value: op.value().get('value').asInt() }));
    op.reply();
  });

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving ${PV} = 100. Ctrl+C to stop.`);

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
