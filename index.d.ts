/** Type declarations for the CommonJS package `node-epics-pvxs`. */

export function version(): string;
export function versionInt(): number;
export function versionAbi(): number;

export interface Value {
  /** Missing field → null; otherwise a Value (use asInt/asString/...). */
  get(name: string): Value | null;
  /** Array fields accept only a JavaScript Array (not Buffer/TypedArray). */
  set(name: string, value: unknown): void;
  assign(value: unknown): void;
  /** Structs → object; scalar/array leaves → number/string/boolean/Array. */
  toObject(): Record<string, unknown> | string | number | boolean | unknown[] | null;
  asBool(): boolean;
  asInt(): number;
  asFloat(): number;
  asString(): string;
  asArray(): unknown[];
  clone(): Value;
  cloneEmpty(): Value;
  equalInst(other: Value): boolean;
  equalType(other: Value): boolean;
  id(): string;
  valid(): boolean;
  nmembers(): number;
  storageType(): number;
  typeCode(): number;
  toString(): string;
}

/** Field object or Value accepted by put / putMany. */
export type PutPayload = Value | Record<string, unknown>;

export interface TypeCodeMap {
  Bool: number;
  BoolA: number;
  Int8: number;
  Int16: number;
  Int32: number;
  Int64: number;
  UInt8: number;
  UInt16: number;
  UInt32: number;
  UInt64: number;
  Int8A: number;
  Int16A: number;
  Int32A: number;
  Int64A: number;
  UInt8A: number;
  UInt16A: number;
  UInt32A: number;
  UInt64A: number;
  Float32: number;
  Float64: number;
  Float32A: number;
  Float64A: number;
  String: number;
  StringA: number;
  Struct: number;
  Union: number;
  Any: number;
  StructA: number;
  UnionA: number;
  AnyA: number;
  Null: number;
}

export interface StoreTypeMap {
  Null: number;
  Bool: number;
  UInteger: number;
  Integer: number;
  Real: number;
  String: number;
  Compound: number;
  Array: number;
}

export interface NTScalarOptions {
  display?: boolean;
  control?: boolean;
  valueAlarm?: boolean;
  form?: boolean;
}

export interface NTScalarApi {
  create(typeCode: number, initial?: Record<string, unknown>, options?: NTScalarOptions): Value;
}

export interface NTEnumApi {
  create(initial?: Record<string, unknown>): Value;
}

export interface MemberSpec {
  code: number;
  name: string;
  children?: MemberSpec[];
}

export interface TypeDefApi {
  create(typeCode: number, members?: MemberSpec[]): Value;
}

export interface DataModule {
  Value: { new (): Value };
  TypeCode: TypeCodeMap;
  StoreType: StoreTypeMap;
  Member: (code: number, name: string, children?: MemberSpec[]) => MemberSpec;
  TypeDef: TypeDefApi;
  NTScalar: NTScalarApi;
  NTEnum: NTEnumApi;
}

/** Monitor/discover stream events (use `instanceof client.Disconnected`, etc.). */
export interface ClientEvent {}

export interface RemoteError extends ClientEvent {
  readonly message: string;
}

export interface Connected extends ClientEvent {
  readonly peerName: string;
  readonly message: string;
}

export interface Disconnected extends ClientEvent {}
export interface Finished extends ClientEvent {}
export interface Interrupted extends ClientEvent {}
export interface Timeout extends ClientEvent {}

export interface Discovered {
  event: number;
  peerVersion: string;
  peer: string;
  proto: string;
  server: string;
}

/** Events yielded by pull/push monitor. */
export type MonitorEvent =
  | Value
  | RemoteError
  | Connected
  | Disconnected
  | Finished
  | Interrupted
  | Timeout;

/** Events yielded by discover (Finished ends the iterator). */
export type DiscoverEvent = Discovered | Finished | Interrupted | Timeout | RemoteError;

/** Pull-mode monitor/discover handle: both async iterable and async iterator. */
export interface AsyncIterableHandle<T> extends AsyncIterable<T>, AsyncIterator<T, undefined, undefined> {
  next(): Promise<IteratorResult<T, undefined>>;
  cancel(): void;
  name(): string;
  pause?(): void;
  resume?(): void;
}

/** Push-mode monitor handle (not async-iterable). */
export interface CallbackSubscriptionHandle {
  cancel(): void;
  name(): string;
  pause?(): void;
  resume?(): void;
}

export interface ClientContext {
  close(): void;
  /** timeout: seconds (PVXS / p4p). Default 5. */
  get(name: string, timeout?: number): Promise<Value> & { cancel(): void };
  /** Batch get; prefer putMany for multi-put. */
  get(names: string[], timeout?: number): Promise<Value[]>;
  put(
    name: string,
    data: PutPayload,
    timeout?: number
  ): Promise<Value> & { cancel(): void };
  /** Batch put: name → payload. One failure rejects the whole batch. */
  putMany(
    values: Record<string, PutPayload>,
    timeout?: number
  ): Promise<Record<string, Value>>;
  rpc(
    name: string,
    args?: Record<string, unknown>,
    timeout?: number
  ): Promise<Value> & { cancel(): void };
  /** List PV names on one peer (`server` = IP or IP:port). Result NTScalarArray field `value`. */
  list(server: string, timeout?: number): Promise<Value> & { cancel(): void };
  /** Pull-mode: Finished ends the iterator; Disconnected does not. Prefer `instanceof client.Disconnected` etc. Connected is masked (p4p). First event is synthetic Disconnected. */
  monitor(name: string): AsyncIterableHandle<MonitorEvent>;
  /**
   * Push-mode. onError: callback throws / next() failure (default console.error).
   * Callback throws do not end the subscription.
   */
  monitor(
    name: string,
    callback: (event: MonitorEvent) => void,
    onError?: (err: unknown) => void
  ): CallbackSubscriptionHandle;
  /** Yields Discovered updates; Finished ends the iterator (e.g. after cancel). */
  discover(options?: { ping?: boolean }): AsyncIterableHandle<DiscoverEvent>;
}

/** Public Context factory (no `new`; use fromEnv / fromConfig). */
export interface ClientContextConstructor {
  fromEnv(): ClientContext;
  /** Config instance or plain EPICS_PVA_* defs object. */
  fromConfig(config: ClientConfig | Record<string, string | number | boolean>): ClientContext;
}

export interface ClientConfig {
  // Opaque handle for pvxs::client::Config
}

export interface ClientConfigConstructor {
  new (): ClientConfig;
  fromEnv(): ClientConfig;
  fromDefs(defs: Record<string, string | number | boolean>): ClientConfig;
}

export interface ClientModule {
  Context: ClientContextConstructor;
  Config: ClientConfigConstructor;
  /** Native subscription class (prefer Context.monitor). */
  Subscription: unknown;
  /** Native discover class (prefer Context.discover). */
  Discover: unknown;
  RemoteError: new (message: string) => RemoteError;
  Connected: new (peerName: string) => Connected;
  Disconnected: new () => Disconnected;
  Finished: new () => Finished;
  Interrupted: new () => Interrupted;
  Timeout: new () => Timeout;
}

export const data: DataModule;
export const client: ClientModule;

export interface ExecOp {
  reply(value?: Value): void;
  error(message?: string): void;
  value(): Value;
}

export interface SharedPV {
  /** Requires a Value (e.g. NTScalar.create); plain objects are not accepted. */
  open(initial: Value): void;
  /** Avoid while put/RPC on this PV may still be in progress. */
  close(): void;
  isOpen(): boolean;
  /** Value, or plain object merged onto the current value. */
  post(value: Value | Record<string, unknown>): void;
  current(): Value;
  /** Synchronous: must call op.reply()/op.error() before return. */
  onPut(callback: (pv: SharedPV, op: ExecOp) => void): void;
  /** Synchronous: must call op.reply()/op.error() before return. */
  onRPC(callback: (pv: SharedPV, op: ExecOp) => void): void;
}

export interface SharedPVConstructor {
  new (): SharedPV;
  buildMailbox(): SharedPV;
  buildReadonly(): SharedPV;
}

export interface StaticSource {
  add(name: string, pv: SharedPV): void;
  remove(name: string): void;
  list(): Record<string, SharedPV>;
  close(): void;
}

export interface StaticSourceConstructor {
  new (): StaticSource;
  build(): StaticSource;
}

export interface Server {
  addPV(name: string, pv: SharedPV): void;
  removePV(name: string): void;
  addSource(name: string, source: StaticSource): void;
  removeSource(name: string): void;
  start(): void;
  stop(): void;
  clientConfig(): ClientConfig;
}

export interface ServerConstructor {
  new (): Server;
  fromEnv(): Server;
  fromIsolated(): Server;
}

export interface ServerModule {
  ExecOp: new () => ExecOp;
  SharedPV: SharedPVConstructor;
  StaticSource: StaticSourceConstructor;
  Server: ServerConstructor;
}

export const server: ServerModule;
