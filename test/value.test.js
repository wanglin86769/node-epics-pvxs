'use strict';

const assert = require('assert');
const pvxs = require('..');

const { TypeCode, NTScalar, NTEnum } = pvxs.data;

function approx(a, b, eps = 1e-6) {
  return Math.abs(a - b) <= eps;
}

function main() {
  // --- integer scalars
  const integerScalars = [
    [TypeCode.UInt8, 255],
    [TypeCode.UInt16, 65535],
    [TypeCode.UInt32, 4294967295],
    [TypeCode.Int8, -128],
    [TypeCode.Int16, -32768],
    [TypeCode.Int32, -2147483648],
    [TypeCode.Int64, -9007199254740991],
  ];

  for (const [code, expected] of integerScalars) {
    const ntValue = NTScalar.create(code);
    ntValue.set('value', expected);
    assert.strictEqual(ntValue.get('value').asInt(), expected);
  }

  // --- float scalars
  for (const [code, expected] of [
    [TypeCode.Float32, -42.2411167],
    [TypeCode.Float64, -42.24111000167],
  ]) {
    const ntValue = NTScalar.create(code);
    ntValue.set('value', expected);
    assert.ok(approx(ntValue.get('value').asFloat(), expected, 1e-4));
  }

  // --- string
  {
    const sample = 'Hello, 👋';
    const ntValue = NTScalar.create(TypeCode.String);
    ntValue.set('value', sample);
    assert.strictEqual(ntValue.get('value').asString(), sample);
  }

  // --- bool
  for (const expected of [false, true]) {
    const ntValue = NTScalar.create(TypeCode.Bool);
    ntValue.set('value', expected);
    assert.strictEqual(ntValue.get('value').asBool(), expected);
  }

  // --- enum struct
  {
    const ntValue = NTEnum.create({
      value: { index: 2, choices: ['zero', 'one', 'two', 'three'] },
    });
    const valueField = ntValue.get('value');
    assert.strictEqual(valueField.get('index').asInt(), 2);
    assert.deepStrictEqual(valueField.get('choices').asArray(), ['zero', 'one', 'two', 'three']);
  }

  // --- int arrays from Array
  for (const [code, expected] of [
    [TypeCode.Int32A, [128, 256, 512]],
    [TypeCode.Float64A, [-42.24111000167, -42.24111000167, -42.24111000167]],
  ]) {
    const ntValue = NTScalar.create(code);
    ntValue.set('value', expected);
    const list = ntValue.get('value').asArray();
    assert.strictEqual(list.length, expected.length);
    for (let i = 0; i < expected.length; i++) {
      assert.ok(approx(Number(list[i]), expected[i], 1e-4));
    }
  }

  // --- array fields reject Buffer / TypedArray
  {
    const ntValue = NTScalar.create(TypeCode.Int32A);
    assert.throws(() => ntValue.set('value', Buffer.from([1, 0, 0, 0])), /Array fields require/);
    assert.throws(() => ntValue.set('value', new Int32Array([1, 2, 3])), /Array fields require/);
  }

  // --- string array
  {
    const sample = ['Hello, 👋', 'from', 'node'];
    const ntValue = NTScalar.create(TypeCode.StringA);
    ntValue.set('value', sample);
    assert.deepStrictEqual(ntValue.get('value').asArray(), sample);
  }

  // --- assign dictionary (NTEnum metadata)
  {
    const sample = {
      value: { index: 1, choices: ['OFF', 'ON'] },
      display: { description: 'sample description' },
      timeStamp: { secondsPastEpoch: 167555999, nanoseconds: 500, userTag: 0 },
    };
    const ntValue = NTEnum.create();
    ntValue.assign(sample);
    const obj = ntValue.toObject();
    assert.deepStrictEqual(obj.value, sample.value);
    assert.deepStrictEqual(obj.display, sample.display);
    assert.deepStrictEqual(obj.timeStamp, sample.timeStamp);
  }

  // --- get missing field
  {
    const ntValue = NTScalar.create(TypeCode.String);
    ntValue.set('value', 'ok');
    assert.strictEqual(ntValue.get('value').asString(), 'ok');
    assert.strictEqual(ntValue.get('missing'), null);
    assert.deepStrictEqual(ntValue.get('missing') ?? {}, {});
    assert.strictEqual(ntValue.valid(), true);
    assert.ok(ntValue.nmembers() > 0);
    assert.strictEqual(ntValue.get('value').nmembers(), 0);
    assert.strictEqual(ntValue.get('value').storageType(), pvxs.data.StoreType.String);
  }

  // --- equality
  {
    const sample = {
      value: { index: 1, choices: ['OFF', 'ON'] },
      display: { description: 'sample description' },
      timeStamp: { secondsPastEpoch: 167555999, nanoseconds: 500, userTag: 0 },
    };
    const a = NTEnum.create();
    a.assign(sample);
    const b = NTEnum.create();
    b.assign(sample);
    const c = NTEnum.create();
    c.assign(sample);
    c.set('timeStamp', { secondsPastEpoch: 167555999, nanoseconds: 0 });

    assert.strictEqual(a.equalInst(a), true);
    assert.strictEqual(a.equalInst(c), false);
    assert.strictEqual(a.equalType(b), true);
    assert.notDeepStrictEqual(a.toObject(), c.toObject());
  }

  // --- clone copies values; mutations do not affect the original
  {
    const ntValue = NTScalar.create(TypeCode.Int32);
    ntValue.set('value', 42);
    const copy = ntValue.clone();
    assert.strictEqual(copy.get('value').asInt(), 42);
    assert.strictEqual(copy.equalType(ntValue), true);
    assert.strictEqual(copy.equalInst(ntValue), false);
    copy.set('value', 99);
    assert.strictEqual(ntValue.get('value').asInt(), 42);
    assert.strictEqual(copy.get('value').asInt(), 99);
    const empty = ntValue.cloneEmpty();
    assert.strictEqual(empty.equalType(ntValue), true);
    assert.notStrictEqual(empty.get('value').asInt(), 42);
  }

  // --- toObject: struct → object; leaf → plain scalar/array
  {
    const ntValue = NTScalar.create(TypeCode.Int32);
    ntValue.set('value', 42);
    assert.strictEqual(ntValue.get('value').toObject(), 42);
    assert.strictEqual(ntValue.toObject().value, 42);

    const arr = NTScalar.create(TypeCode.Int32A);
    arr.set('value', [1, 2, 3]);
    assert.deepStrictEqual(arr.get('value').toObject(), [1, 2, 3]);
  }

  // --- bool array
  {
    const sample = [true, false, true, false];
    const ntValue = NTScalar.create(TypeCode.BoolA);
    ntValue.set('value', sample);
    assert.deepStrictEqual(ntValue.get('value').asArray(), sample);
  }

  // --- custom struct via TypeDef
  {
    const { TypeDef, Member } = pvxs.data;
    const val = TypeDef.create(TypeCode.Struct, [
      Member(TypeCode.String, 'desc'),
      Member(TypeCode.Bool, 'flag'),
      Member(TypeCode.Int16, 'number32'),
      Member(TypeCode.Int64A, 'array64'),
      Member(TypeCode.Struct, 'substruct', [
        Member(TypeCode.Bool, 'flag'),
        Member(TypeCode.Int16, 'number32'),
        Member(TypeCode.Int64A, 'array64'),
      ]),
    ]);

    val.set('desc', 'some string');
    val.set('flag', true);
    val.set('number32', 999);
    val.get('substruct').set('flag', false);
    val.get('substruct').set('number32', -888);
    val.get('substruct').set('array64', [1, 2, 3, 4, 5]);

    assert.strictEqual(val.get('desc').asString(), 'some string');
    assert.strictEqual(val.get('flag').asBool(), true);
    assert.strictEqual(val.get('number32').asInt(), 999);
    assert.deepStrictEqual(val.get('substruct').get('array64').asArray(), [1, 2, 3, 4, 5]);
  }

  console.log('value.test.js: ok');
}

try {
  main();
} catch (err) {
  console.error(err);
  process.exitCode = 1;
}
