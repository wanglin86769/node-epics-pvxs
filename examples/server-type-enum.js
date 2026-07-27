/**
 * NTEnum (Server.fromEnv). Writable via onPut.
 *
 * Run:  node examples/server-type-enum.js
 * Stop: Ctrl+C
 *
 * While running (choices: OFF=0, ON=1, FAULT=2; initial index=1 ON):
 *   pvget nodepvxs:ex:t:mode
 *   pvput nodepvxs:ex:t:mode FAULT
 *   pvput nodepvxs:ex:t:mode 0
 *   pvput nodepvxs:ex:t:mode value.index=2
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { NTEnum } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:t:mode';

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
  pv.open(
    NTEnum.create(
      withTimeStamp({
        value: { index: 1, choices: ['OFF', 'ON', 'FAULT'] },
      })
    )
  );
  pv.onPut((p, op) => {
    const v = op.value();
    const unixSec = Date.now() / 1000;
    const sec = Math.floor(unixSec);
    const ns = Math.floor((unixSec - sec) * 1e9);
    v.set('timeStamp', {
      secondsPastEpoch: sec,
      nanoseconds: ns,
      userTag: 0,
    });
    p.post(v);
    op.reply();
  });

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving writable ${PV} (NTEnum, index=1 ON). Ctrl+C to stop.`);
  console.log(`  pvget ${PV}`);
  console.log(`  pvput ${PV} FAULT`);
  console.log(`  pvput ${PV} 0`);
  console.log(`  pvput ${PV} value.index=2`);

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
