# Changelog

## 0.1.0

- **data:** `Value`, `TypeCode`, `StoreType`, `Member`, `TypeDef`, `NTScalar`, `NTEnum`
- **client:** `Context` (`fromEnv` / config); async `get` (single or batch), `put`, `putMany`, `rpc`, `list`; `monitor` (`for await` or callback); `discover`; cancelable Promises and timeout
- **server:** `SharedPV`, `StaticSource`, `Server`, `ExecOp` (synchronous `onPut` / `onRPC`)
- N-API native addon on pvxs (`node-addon-api` + `node-gyp`)
- TypeScript declarations (`index.d.ts`)
- Examples under `examples/`; integration tests under `test/`
- Local builds via `config/epics-paths.js` and `scripts/resolve-epics-paths.js`
- Prebuilds only: `prebuilds/<platform>-<arch>/` with `pvxs.node` and bundled shared libs (`win32-x64`, `linux-x64`, `darwin-x64`)
- No `postinstall`; `"gypfile": false` (install does not run `node-gyp rebuild`)
- Server `onPut` / `onRPC` must call `op.reply()` or `op.error()` before return
