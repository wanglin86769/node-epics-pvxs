'use strict';

const assert = require('assert');
const { createClient } = require('../lib/client');

class Finished {
  constructor() {
    this.tag = 'Finished';
  }
}

class Disconnected {
  constructor() {
    this.tag = 'Disconnected';
  }
}

class Value {
  constructor(n) {
    this.n = n;
  }
}

function mockHandle(events) {
  let i = 0;
  let cancelled = false;
  const waiters = [];

  function flush() {
    while (waiters.length > 0 && (cancelled || i < events.length)) {
      const resolve = waiters.shift();
      if (cancelled && i >= events.length) {
        resolve(new Finished());
        continue;
      }
      resolve(events[i++]);
    }
  }

  return {
    next() {
      return new Promise((resolve) => {
        waiters.push(resolve);
        flush();
      });
    },
    cancel() {
      cancelled = true;
      flush();
    },
    pause() {},
    resume() {},
    name() {
      return 'mock:pv';
    },
  };
}

function mockClient(events) {
  const nativeCtx = {
    close() {},
    monitor() {
      return mockHandle(events);
    },
    discover() {
      return mockHandle([]);
    },
  };
  return createClient({
    Config: {},
    Finished,
    Disconnected,
    Context: {
      fromEnv() {
        return nativeCtx;
      },
      fromConfig() {
        return nativeCtx;
      },
    },
  });
}

async function testCallbackDispatchDeliversUntilFinished() {
  const seen = [];
  const ctx = mockClient([
    new Value(1),
    new Disconnected(),
    new Value(2),
    new Finished(),
  ]).Context.fromEnv();

  await new Promise((resolve) => {
    ctx.monitor('mock:pv', (ev) => {
      seen.push(ev);
      if (ev instanceof Finished) {
        resolve();
      }
    });
  });

  assert.strictEqual(seen.length, 4);
  assert.ok(seen[0] instanceof Value && seen[0].n === 1);
  assert.ok(seen[1] instanceof Disconnected);
  assert.ok(seen[2] instanceof Value && seen[2].n === 2);
  assert.ok(seen[3] instanceof Finished);
}

async function testCancelSkipsFinishedCallback() {
  const seen = [];
  const ctx = mockClient([new Value(1)]).Context.fromEnv();

  let resolveFirst;
  const gotFirst = new Promise((resolve) => {
    resolveFirst = resolve;
  });

  const sub = ctx.monitor('mock:pv', (ev) => {
    seen.push(ev);
    if (ev instanceof Value) {
      resolveFirst();
    }
  });

  await gotFirst;
  assert.strictEqual(seen.length, 1);
  assert.ok(seen[0] instanceof Value);

  sub.cancel();

  // Allow any pending dispatch turn to run after cancel.
  await Promise.resolve();
  await Promise.resolve();

  assert.ok(seen.every((ev) => !(ev instanceof Finished)));
}

async function testAsyncIterableEndsOnFinishedOnly() {
  const ctx = mockClient([
    new Value(1),
    new Disconnected(),
    new Finished(),
  ]).Context.fromEnv();

  const iter = ctx.monitor('mock:pv');

  const a = await iter.next();
  assert.strictEqual(a.done, false);
  assert.ok(a.value instanceof Value);

  const b = await iter.next();
  assert.strictEqual(b.done, false);
  assert.ok(b.value instanceof Disconnected);

  const c = await iter.next();
  assert.strictEqual(c.done, true);
  assert.ok(c.value instanceof Finished);
}

function testCreateClientHidesNativeContext() {
  const nativeContext = function NativeContext() {};
  const client = createClient({
    Config: {},
    Context: nativeContext,
  });
  assert.notStrictEqual(client.Context, nativeContext);
  assert.strictEqual(typeof client.Context.fromEnv, 'function');
  assert.strictEqual(typeof client.Context.fromConfig, 'function');
  assert.ok(client.Config);
}

async function testOnErrorReceivesCallbackThrow() {
  const errors = [];
  const ctx = mockClient([new Value(1), new Value(2), new Finished()]).Context.fromEnv();

  await new Promise((resolve) => {
    ctx.monitor(
      'mock:pv',
      (ev) => {
        if (ev instanceof Value && ev.n === 1) {
          throw new Error('boom');
        }
        if (ev instanceof Finished) {
          resolve();
        }
      },
      (err) => {
        errors.push(err);
      }
    );
  });

  assert.strictEqual(errors.length, 1);
  assert.strictEqual(errors[0].message, 'boom');
}

async function main() {
  await testCallbackDispatchDeliversUntilFinished();
  await testCancelSkipsFinishedCallback();
  await testAsyncIterableEndsOnFinishedOnly();
  testCreateClientHidesNativeContext();
  await testOnErrorReceivesCallbackThrow();
  console.log('client-monitor.test.js: ok');
}

main().catch((err) => {
  console.error(err);
  process.exitCode = 1;
});
