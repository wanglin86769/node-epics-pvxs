/**
 * StaticSource + addSource (Server.fromEnv).
 *
 * Run:  node examples/server-static-source.js
 * Stop: Ctrl+C
 *
 * While running:
 *   pvget nodepvxs:ex:box:a
 *   pvget nodepvxs:ex:box:b
 *   pvput nodepvxs:ex:box:a 9
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, StaticSource, Server } = pvxs.server;

const PV_A = 'nodepvxs:ex:box:a';
const PV_B = 'nodepvxs:ex:box:b';

function withTimeStamp(fields) {
  const unixSec = Date.now() / 1000;
  const sec = Math.floor(unixSec);
  const ns = Math.floor((unixSec - sec) * 1e9);
  return Object.assign({}, fields, {
    timeStamp: { secondsPastEpoch: sec, nanoseconds: ns, userTag: 0 },
  });
}

async function main() {
  const a = SharedPV.buildMailbox();
  a.open(NTScalar.create(TypeCode.Int32, withTimeStamp({ value: 1 })));
  a.onPut((p, op) => {
    p.post(withTimeStamp({ value: op.value().get('value').asInt() }));
    op.reply();
  });

  const b = SharedPV.buildMailbox();
  b.open(NTScalar.create(TypeCode.String, withTimeStamp({ value: 'x' })));

  const source = StaticSource.build();
  source.add(PV_A, a);
  source.add(PV_B, b);

  const server = Server.fromEnv();
  server.addSource('box', source);
  server.start();

  console.log(`serving via StaticSource: ${Object.keys(source.list()).join(', ')}`);
  console.log('Ctrl+C to stop.');
  console.log(`  pvget ${PV_A}`);
  console.log(`  pvget ${PV_B}`);
  console.log(`  pvput ${PV_A} 9`);

  const shutdown = () => {
    server.stop();
    source.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
