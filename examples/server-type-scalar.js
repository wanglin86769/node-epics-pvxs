/**
 * NTScalar scalars: Int32, Float64, String, Bool (Server.fromEnv).
 * Writable via onPut.
 *
 * Run:  node examples/server-type-scalar.js
 * Stop: Ctrl+C
 *
 * While running:
 *   pvget nodepvxs:ex:t:int
 *   pvget nodepvxs:ex:t:float
 *   pvget nodepvxs:ex:t:str
 *   pvget nodepvxs:ex:t:bool
 *   pvput nodepvxs:ex:t:int 42
 *   pvput nodepvxs:ex:t:float 3.14
 *   pvput nodepvxs:ex:t:str hello
 *   pvput nodepvxs:ex:t:bool false
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

function withTimeStamp(fields) {
  const unixSec = Date.now() / 1000;
  const sec = Math.floor(unixSec);
  const ns = Math.floor((unixSec - sec) * 1e9);
  return Object.assign({}, fields, {
    timeStamp: { secondsPastEpoch: sec, nanoseconds: ns, userTag: 0 },
  });
}

function mailbox(code, initial) {
  const pv = SharedPV.buildMailbox();
  pv.open(NTScalar.create(code, withTimeStamp({ value: initial })));
  pv.onPut((p, op) => {
    p.post(op.value());
    op.reply();
  });
  return pv;
}

async function main() {
  const intPv = mailbox(TypeCode.Int32, -7);
  const fPv = mailbox(TypeCode.Float64, 1.5);
  const sPv = mailbox(TypeCode.String, 'hi');
  const bPv = mailbox(TypeCode.Bool, true);

  const server = Server.fromEnv();
  server.addPV('nodepvxs:ex:t:int', intPv);
  server.addPV('nodepvxs:ex:t:float', fPv);
  server.addPV('nodepvxs:ex:t:str', sPv);
  server.addPV('nodepvxs:ex:t:bool', bPv);
  server.start();

  console.log('serving writable scalar PVs. Ctrl+C to stop.');
  console.log('  pvget nodepvxs:ex:t:int');
  console.log('  pvget nodepvxs:ex:t:float');
  console.log('  pvget nodepvxs:ex:t:str');
  console.log('  pvget nodepvxs:ex:t:bool');
  console.log('  pvput nodepvxs:ex:t:int 42');
  console.log('  pvput nodepvxs:ex:t:float 3.14');
  console.log('  pvput nodepvxs:ex:t:str hello');
  console.log('  pvput nodepvxs:ex:t:bool false');

  const shutdown = () => {
    server.stop();
    intPv.close();
    fPv.close();
    sPv.close();
    bPv.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
