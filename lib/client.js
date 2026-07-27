/** Public `pvxs.client`: JS Context (batch / monitor sugar) over the N-API binding. */
function createClient(nativeClient) {
  const NativeContext = nativeClient.Context;
  const Finished = nativeClient.Finished;
  const client = {};
  for (const key of Object.keys(nativeClient)) {
    if (key === 'Context') {
      continue;
    }
    client[key] = nativeClient[key];
  }
  client.Context = {
    fromEnv() {
      return attachApi(NativeContext.fromEnv(), Finished);
    },
    fromConfig(config) {
      return attachApi(NativeContext.fromConfig(config), Finished);
    },
  };
  return client;
}

/** Attach the public Context instance API to a native context handle. */
function attachApi(nativeCtx, Finished) {
  return {
    close() {
      nativeCtx.close();
    },
    get(name, timeout) {
      if (Array.isArray(name)) {
        return Promise.all(
          name.map((item) =>
            timeout === undefined ? nativeCtx.get(item) : nativeCtx.get(item, timeout)
          )
        );
      }
      return timeout === undefined ? nativeCtx.get(name) : nativeCtx.get(name, timeout);
    },
    put(name, data, timeout) {
      return timeout === undefined
        ? nativeCtx.put(name, data)
        : nativeCtx.put(name, data, timeout);
    },
    async putMany(values, timeout) {
      if (values === null || typeof values !== 'object' || Array.isArray(values)) {
        throw new TypeError('putMany(values[, timeout]) requires a plain object map');
      }
      // Each put uses fetchPresent(true): server present value is fetched, then payload applied.
      const names = Object.keys(values);
      const results = await Promise.all(
        names.map((name) =>
          timeout === undefined
            ? nativeCtx.put(name, values[name])
            : nativeCtx.put(name, values[name], timeout)
        )
      );
      const out = {};
      for (let i = 0; i < names.length; i++) {
        out[names[i]] = results[i];
      }
      return out;
    },
    rpc(name, args, timeout) {
      if (args === undefined) {
        return nativeCtx.rpc(name);
      }
      return timeout === undefined
        ? nativeCtx.rpc(name, args)
        : nativeCtx.rpc(name, args, timeout);
    },
    list(server, timeout) {
      return timeout === undefined
        ? nativeCtx.list(server)
        : nativeCtx.list(server, timeout);
    },
    /**
     * monitor(name) — async iterable (for await / next).
     * monitor(name, callback[, onError]) — push events to callback; do not also iterate.
     * onError(err) — optional; called if callback throws or dispatch fails (next() rejects).
     */
    monitor(name, callback, onError) {
      const handle = nativeCtx.monitor(name);
      if (callback === undefined) {
        return asAsyncIterable(handle, Finished);
      }
      if (typeof callback !== 'function') {
        handle.cancel();
        throw new TypeError('monitor(name, callback) requires callback to be a function');
      }
      if (onError !== undefined && typeof onError !== 'function') {
        handle.cancel();
        throw new TypeError('monitor(name, callback, onError) onError must be a function');
      }
      return asCallbackSubscription(handle, callback, onError, Finished);
    },
    discover(options) {
      return asAsyncIterable(nativeCtx.discover(options), Finished);
    },
    _native: nativeCtx,
  };
}

/** Async iterable over monitor/discover: Finished ends the loop; Disconnected does not. */
function asAsyncIterable(handle, Finished) {
  const iterable = {
    [Symbol.asyncIterator]() {
      return iterable;
    },
    async next() {
      const value = await handle.next();
      const done = isTerminalEvent(value, Finished);
      return { value, done };
    },
    cancel() {
      handle.cancel();
    },
    pause() {
      if (typeof handle.pause === 'function') {
        handle.pause();
      }
    },
    resume() {
      if (typeof handle.resume === 'function') {
        handle.resume();
      }
    },
    name() {
      return handle.name();
    },
  };
  return iterable;
}

/** Callback monitor: pull next() and invoke callback (not async-iterable). */
function asCallbackSubscription(handle, callback, onError, Finished) {
  let stopped = false;

  const subscription = {
    cancel() {
      if (stopped) {
        return;
      }
      stopped = true;
      handle.cancel();
    },
    pause() {
      if (typeof handle.pause === 'function') {
        handle.pause();
      }
    },
    resume() {
      if (typeof handle.resume === 'function') {
        handle.resume();
      }
    },
    name() {
      return handle.name();
    },
  };

  void (async () => {
    try {
      while (!stopped) {
        const value = await handle.next();
        if (stopped) {
          return;
        }
        try {
          callback(value);
        } catch (err) {
          reportMonitorError(err, onError);
        }
        if (isTerminalEvent(value, Finished)) {
          stopped = true;
          return;
        }
      }
    } catch (err) {
      if (!stopped) {
        stopped = true;
        reportMonitorError(err, onError);
      }
    }
  })();

  return subscription;
}

function reportMonitorError(err, onError) {
  if (typeof onError === 'function') {
    try {
      onError(err);
    } catch (handlerErr) {
      console.error('[node-epics-pvxs] monitor onError handler threw:', handlerErr);
    }
    return;
  }
  console.error('[node-epics-pvxs] monitor error:', err);
}

function isTerminalEvent(value, Finished) {
  return typeof Finished === 'function' && value instanceof Finished;
}

module.exports = {
  createClient,
};
