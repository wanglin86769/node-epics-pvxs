'use strict';

const assert = require('assert');
const pvxs = require('..');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server, StaticSource } = pvxs.server;

async function main() {
  const intPv = SharedPV.buildMailbox();
  intPv.open(NTScalar.create(TypeCode.Int32, { value: -42 }));

  const strPv = SharedPV.buildMailbox();
  strPv.onPut((pv, op) => {
    pv.post(op.value());
    op.reply();
  });
  strPv.onRPC((pv, op) => {
    op.reply(op.value());
  });
  strPv.open(NTScalar.create(TypeCode.String, { value: 'minus forty-two' }));

  const roPv = SharedPV.buildReadonly();
  roPv.open(NTScalar.create(TypeCode.Int32, { value: 7 }));

  const asyncPv = SharedPV.buildMailbox();
  asyncPv.onPut(async () => {
    // Intentionally async — must be rejected by the binding.
  });
  asyncPv.open(NTScalar.create(TypeCode.Int32, { value: 1 }));

  const server = Server.fromIsolated();
  server.addPV('scalar_int32', intPv);
  server.addPV('scalar_string', strPv);
  server.addPV('scalar_readonly', roPv);
  server.addPV('scalar_async', asyncPv);
  server.start();

  try {
    const client = pvxs.client.Context.fromConfig(server.clientConfig());

    const got = await client.get('scalar_int32');
    assert.strictEqual(got.get('value').asInt(), -42);

    await client.put('scalar_string', { value: 'minus forty-three', 'alarm.message': 'OK' });
    const afterPut = await client.get('scalar_string');
    assert.strictEqual(afterPut.get('value').asString(), 'minus forty-three');

    const rpcVal = await client.rpc('scalar_string', {
      some_float: 999.9,
      some_string: 'a string',
      some_flag: true,
    });
    assert.strictEqual(rpcVal.get('query').get('some_float').asFloat(), 999.9);
    assert.strictEqual(rpcVal.get('query').get('some_flag').asBool(), true);

    const posted = strPv.current();
    assert.strictEqual(posted.get('value').asString(), 'minus forty-three');

    const roGot = await client.get('scalar_readonly');
    assert.strictEqual(roGot.get('value').asInt(), 7);

    await assert.rejects(
      () => client.put('scalar_readonly', { value: 99 }),
      /Read-only/
    );
    assert.strictEqual(roPv.current().get('value').asInt(), 7);

    await assert.rejects(
      () => client.put('scalar_async', { value: 2 }),
      /synchronous|Promise|async/i
    );
    assert.strictEqual(asyncPv.current().get('value').asInt(), 1);

    // Server keeps JS SharedPV alive after local refs are dropped.
    {
      const pv = SharedPV.buildMailbox();
      pv.onPut((p, op) => {
        p.post(op.value());
        op.reply();
      });
      pv.open(NTScalar.create(TypeCode.Int32, { value: 0 }));
      server.addPV('held_by_server', pv);
    }
    await client.put('held_by_server', { value: 5 });
    assert.strictEqual(
      (await client.get('held_by_server')).get('value').asInt(),
      5
    );

    // StaticSource + addSource keep both source and SharedPV wraps alive.
    {
      const pv = SharedPV.buildMailbox();
      pv.onPut((p, op) => {
        p.post(op.value());
        op.reply();
      });
      pv.open(NTScalar.create(TypeCode.Int32, { value: 10 }));
      const source = StaticSource.build();
      source.add('held_by_source', pv);
      assert.strictEqual(source.list().held_by_source, pv);
      server.addSource('box', source);
    }
    await client.put('held_by_source', { value: 11 });
    assert.strictEqual(
      (await client.get('held_by_source')).get('value').asInt(),
      11
    );

    server.removeSource('box');
    // Same source name can be registered again only after a successful removeSource.
    {
      const pv = SharedPV.buildMailbox();
      pv.open(NTScalar.create(TypeCode.Int32, { value: 20 }));
      const source = StaticSource.build();
      source.add('readded_pv', pv);
      server.addSource('box', source);
    }

    // JS throw in onPut becomes a clear put error (not a silent miss).
    {
      const pv = SharedPV.buildMailbox();
      pv.onPut(() => {
        throw new Error('handler boom');
      });
      pv.open(NTScalar.create(TypeCode.Int32, { value: 0 }));
      server.addPV('throw_put', pv);
    }
    await assert.rejects(() => client.put('throw_put', { value: 1 }), /handler boom/);

    client.close();
    console.log('server.test.js: ok');
  } finally {
    server.stop();
    intPv.close();
    strPv.close();
    roPv.close();
    asyncPv.close();
  }
  // Native addon handles can keep the event loop alive after stop/close.
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
