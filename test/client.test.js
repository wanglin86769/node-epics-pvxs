'use strict';

const assert = require('assert');
const pvxs = require('..');
const { TypeCode, NTScalar } = pvxs.data;
const { SharedPV, Server } = pvxs.server;

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

  const server = Server.fromIsolated();
  server.addPV('scalar_int32', intPv);
  server.addPV('scalar_string', strPv);
  server.start();

  try {
    {
      const fromEnvCfg = pvxs.client.Config.fromEnv();
      const c1 = pvxs.client.Context.fromConfig(fromEnvCfg);
      c1.close();

      const fromDefsCfg = pvxs.client.Config.fromDefs({
        EPICS_PVA_AUTO_ADDR_LIST: 'YES',
      });
      const c2 = pvxs.client.Context.fromConfig(fromDefsCfg);
      c2.close();

      const c3 = pvxs.client.Context.fromConfig({
        EPICS_PVA_AUTO_ADDR_LIST: true,
      });
      c3.close();
    }

    const client = pvxs.client.Context.fromConfig(server.clientConfig());

    const got = await client.get('scalar_int32');
    assert.strictEqual(got.get('value').asInt(), -42);

    const batch = await client.get(['scalar_int32', 'scalar_string']);
    assert.strictEqual(batch.length, 2);
    assert.strictEqual(batch[0].get('value').asInt(), -42);

    await client.put('scalar_string', { value: 'minus forty-three', 'alarm.message': 'OK' });
    const afterPut = await client.get('scalar_string');
    assert.strictEqual(afterPut.get('value').asString(), 'minus forty-three');

    const batchPut = await client.putMany({
      scalar_int32: { value: -7 },
      scalar_string: { value: 'batch-put' },
    });
    assert.ok(batchPut.scalar_int32);
    assert.ok(batchPut.scalar_string);
    const afterBatchPut = await client.get(['scalar_int32', 'scalar_string']);
    assert.strictEqual(afterBatchPut[0].get('value').asInt(), -7);
    assert.strictEqual(afterBatchPut[1].get('value').asString(), 'batch-put');

    const rpcVal = await client.rpc('scalar_string', {
      some_float: 999.9,
      some_string: 'a string',
      some_flag: true,
    });
    assert.strictEqual(rpcVal.get('query').get('some_float').asFloat(), 999.9);
    assert.strictEqual(rpcVal.get('query').get('some_flag').asBool(), true);

    const fastGet = client.get('scalar_int32', 5);
    assert.strictEqual(typeof fastGet.cancel, 'function');
    await fastGet;

    // Monitor: initial Disconnected, Values, disconnect/reconnect, Finished on cancel.
    {
      const { Disconnected, Finished } = pvxs.client;
      const mon = client.monitor('scalar_int32');
      let sawInitialDisconnect = false;
      let sawValueBeforeStop = false;
      let sawDisconnectAfterStop = false;
      let sawValueAfterRestart = false;
      let stoppedOnce = false;

      await (async () => {
        for await (const ev of mon) {
          if (ev instanceof Disconnected) {
            if (!sawValueBeforeStop) {
              sawInitialDisconnect = true;
              continue;
            }
            if (stoppedOnce && !sawDisconnectAfterStop) {
              sawDisconnectAfterStop = true;
              server.start();
              continue;
            }
            continue;
          }
          if (ev instanceof Finished) {
            break;
          }
          assert.strictEqual(typeof ev.get, 'function');
          if (!stoppedOnce) {
            sawValueBeforeStop = true;
            stoppedOnce = true;
            server.stop();
            continue;
          }
          if (sawDisconnectAfterStop) {
            sawValueAfterRestart = true;
            break;
          }
        }
      })();

      assert.ok(sawInitialDisconnect, 'expected initial Disconnected');
      assert.ok(sawValueBeforeStop, 'expected Value before server stop');
      assert.ok(sawDisconnectAfterStop, 'expected Disconnected after server stop');
      assert.ok(sawValueAfterRestart, 'expected Value after server restart');
      mon.cancel();

      const mon2 = client.monitor('scalar_int32');
      const first = await mon2.next();
      assert.strictEqual(first.done, false);
      assert.ok(
        first.value instanceof Disconnected,
        'expected initial Disconnected on new monitor'
      );
      mon2.cancel();
      const end = await mon2.next();
      assert.strictEqual(end.done, true);
      assert.ok(
        end.value instanceof Finished,
        'expected instanceof Finished after cancel'
      );
    }

    client.close();
    console.log('client.test.js: ok');
  } finally {
    try {
      server.stop();
    } catch (_) {
      /* may already be stopped */
    }
    intPv.close();
    strPv.close();
  }
  // Native addon handles can keep the event loop alive after stop/close.
  process.exit(0);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
