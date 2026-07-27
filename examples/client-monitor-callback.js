/**
 * Client monitor (callback + onError). Runs until Ctrl+C.
 *
 * Terminal A:  node examples/server-post.js
 * Terminal B:  node examples/client-monitor-callback.js
 *
 * First event is Disconnected (subscription starts unconnected); then Values.
 */

'use strict';

const pvxs = require('node-epics-pvxs');

const PV = 'nodepvxs:ex:post';

const client = pvxs.client.Context.fromEnv();
const { Disconnected, Finished } = pvxs.client;

let n = 0;
const sub = client.monitor(
  PV,
  (ev) => {
    if (ev instanceof Disconnected) {
      console.log('[Disconnected]');
      return;
    }
    if (ev instanceof Finished) {
      console.log('[Finished]');
      return;
    }
    n += 1;
    console.log(`#${n}`, ev.get('value').toString());
  },
  (err) => {
    console.error('onError', err);
  }
);

console.log(`callback monitor ${PV} (Ctrl+C to stop)...`);

const shutdown = () => {
  sub.cancel();
  client.close();
  process.exit(0);
};
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
