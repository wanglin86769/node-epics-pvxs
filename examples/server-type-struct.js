/**
 * TypeDef custom struct (Server.fromEnv). Writable via onPut.
 *
 * Run:  node examples/server-type-struct.js
 * Stop: Ctrl+C
 *
 * While running (fields: name, count, meta.ok):
 *   pvget nodepvxs:ex:t:struct
 *   pvput nodepvxs:ex:t:struct count=5
 *   pvput nodepvxs:ex:t:struct name=hello
 *   pvput nodepvxs:ex:t:struct meta.ok=false
 */

'use strict';

const pvxs = require('node-epics-pvxs');
const { TypeCode, TypeDef, Member } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

const PV = 'nodepvxs:ex:t:struct';

async function main() {
  const initial = TypeDef.create(TypeCode.Struct, [
    Member(TypeCode.String, 'name'),
    Member(TypeCode.Int32, 'count'),
    Member(TypeCode.Struct, 'meta', [Member(TypeCode.Bool, 'ok')]),
  ]);
  initial.set('name', 'demo');
  initial.set('count', 1);
  initial.get('meta').set('ok', true);

  const pv = SharedPV.buildMailbox();
  pv.open(initial);
  pv.onPut((p, op) => {
    p.post(op.value());
    op.reply();
  });

  const server = Server.fromEnv();
  server.addPV(PV, pv);
  server.start();

  console.log(`serving writable ${PV} (custom struct). Ctrl+C to stop.`);
  console.log(`  pvget ${PV}`);
  console.log(`  pvput ${PV} count=5`);
  console.log(`  pvput ${PV} name=hello`);
  console.log(`  pvput ${PV} meta.ok=false`);

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
