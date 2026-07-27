# node-epics-pvxs

EPICS [PVAccess](https://epics-base.github.io/pvxs) for Node.js — async client and static server, via an N-API addon on [pvxs](https://github.com/epics-base/pvxs).

API design and many behaviors follow [p4p](https://github.com/epics-base/p4p) and [aiopvxs](https://github.com/m2es3h/aiopvxs).

## Install

```bash
npm install node-epics-pvxs
```

Requires Node.js >= 18. Published builds include `prebuilds/` for **win32-x64**, **linux-x64**, and **darwin-x64** (no compiler needed).

Linux prebuilds need **glibc >= 2.32** (e.g. Debian 12, Ubuntu 22.04+). Older systems such as Debian 10 will fail to load (`GLIBC_2.32 not found`); use a newer OS or [build from source](#build-from-source) on that machine.

## Quick start

### Client

```javascript
const pvxs = require('node-epics-pvxs');

const ctx = pvxs.client.Context.fromEnv();
const v = await ctx.get('some:pv');
console.log(v.get('value').asInt());
await ctx.put('some:pv', { value: 42 });
ctx.close();
```

### Server

```javascript
const pvxs = require('node-epics-pvxs');
const { SharedPV, Server } = pvxs.server;

const pv = SharedPV.buildMailbox();
pv.open(pvxs.data.NTScalar.create(pvxs.data.TypeCode.Int32, { value: 0 }));
// onPut / onRPC must call op.reply() or op.error() synchronously before return.
pv.onPut((p, op) => {
  p.post(op.value());
  op.reply();
});

const server = Server.fromEnv();
server.addPV('demo:pv', pv);
server.start();
```

## Examples

Scripts in `examples/` (included in the npm package):

| Script | Summary |
|--------|---------|
| `server-mailbox.js` | Mailbox SharedPV with a fixed value |
| `server-post.js` | Periodic post, value cycles 0…100 |
| `server-readonly.js` | Readonly SharedPV |
| `server-rpc.js` | onRPC echo |
| `server-put-error.js` | onPut rejects negatives with `op.error` |
| `server-static-source.js` | StaticSource + `addSource` |
| `server-type-scalar.js` | NTScalar Int / Float / String / Bool |
| `server-type-array.js` | NTScalar Int32A / StringA |
| `server-type-enum.js` | NTEnum |
| `server-type-struct.js` | TypeDef custom struct |
| `client-get.js` | get (single + batch) |
| `client-put.js` | put |
| `client-put-many.js` | putMany |
| `client-rpc.js` | rpc |
| `client-monitor.js` | monitor (`for await`) |
| `client-monitor-callback.js` | monitor (callback) |
| `client-list.js` | list PV names on a peer |
| `client-discover.js` | discover peers |
| `client-config.js` | Config.fromEnv / fromDefs / fromConfig |

See `index.d.ts` for types.

## Build from source

1. Copy `config/epics-paths.example.js` → `config/epics-paths.js` and set `pvxsRoot`, `epicsBase`, `epicsHostArch`.
2. `npm run build`
3. Copy `build/Release/pvxs.node` and required shared libs into `prebuilds/<platform>-<arch>/` (see that folder’s README; Linux/macOS normalize scripts under `scripts/`).

Runtime loads **only** from `prebuilds/` (not `build/`).

## License

MIT — see [LICENSE.txt](LICENSE.txt).
