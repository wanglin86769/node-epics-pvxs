/**
 * NTScalar arrays: Int32A, StringA (Server.fromEnv).
 * Array fields use a JS Array (not Buffer / TypedArray).
 * Writable via onPut (accepts client put).
 *
 * Run:  node examples/server-type-array.js
 * Stop: Ctrl+C
 *
 * While running:
 *   pvget nodepvxs:ex:t:ints
 *   pvget nodepvxs:ex:t:strs
 *   pvput nodepvxs:ex:t:ints "[9, 8]"
 *   pvput nodepvxs:ex:t:ints value="[9, 8]"
 *   pvput nodepvxs:ex:t:strs '["x", "y"]'
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV_INTS = 'nodepvxs:ex:t:ints';
const PV_STRS = 'nodepvxs:ex:t:strs';

function withTimeStamp(fields) {
  const unixSec = Date.now() / 1000;
  const sec = Math.floor(unixSec);
  const ns = Math.floor((unixSec - sec) * 1e9);
  return Object.assign({}, fields, {
    timeStamp: { secondsPastEpoch: sec, nanoseconds: ns, userTag: 0 },
  });
}

async function main() {
  const ints = SharedPV.buildMailbox();
  ints.open(NTScalar.create(TypeCode.Int32A, withTimeStamp({ value: [1, 2, 3] })));
  ints.onPut((p, op) => {
    p.post(op.value());
    op.reply();
  });

  const strs = SharedPV.buildMailbox();
  strs.open(NTScalar.create(TypeCode.StringA, withTimeStamp({ value: ['a', 'b'] })));
  strs.onPut((p, op) => {
    p.post(op.value());
    op.reply();
  });

  const server = Server.fromEnv();
  server.addPV(PV_INTS, ints);
  server.addPV(PV_STRS, strs);
  server.start();

  console.log('serving writable array PVs. Ctrl+C to stop.');
  console.log(`  pvget ${PV_INTS}`);
  console.log(`  pvget ${PV_STRS}`);
  console.log(`  pvput ${PV_INTS} "[9, 8]"`);
  console.log(`  pvput ${PV_STRS} '["x", "y"]'`);

  const shutdown = () => {
    server.stop();
    ints.close();
    strs.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
