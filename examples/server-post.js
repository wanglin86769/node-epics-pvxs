/**
 * Server-side post: value cycles 0 → 100 → 0 … (Server.fromEnv).
 *
 * Run:  node examples/server-post.js
 * Stop: Ctrl+C
 *
 * While running: pvmonitor nodepvxs:ex:post
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:post';
const TICK_MS = 1000;

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

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`posting ${PV} every ${TICK_MS}ms (0..100). Ctrl+C to stop.`);

  let n = 0;
  const tick = setInterval(() => {
    pv.post(withTimeStamp({ value: n }));
    console.log('post', n);
    n = n >= 100 ? 0 : n + 1;
  }, TICK_MS);

  const shutdown = () => {
    clearInterval(tick);
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
