/**
 * Public data module: native Value/TypeCode/NT* plus JS Member helper.
 */

function Member(code, name, children) {
  const member = { code, name };
  if (children !== undefined) {
    member.children = children;
  }
  return member;
}

/**
 * Build the public `pvxs.data` object. Adds Member for TypeDef.create() specs.
 */
function createData(nativeData) {
  const data = {};
  for (const key of Object.keys(nativeData)) {
    data[key] = nativeData[key];
  }
  data.Member = Member;
  return data;
}

module.exports = {
  createData,
};
