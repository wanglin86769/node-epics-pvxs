const { loadBinding } = require('./lib/binding');
const { createClient } = require('./lib/client');
const { createData } = require('./lib/data');

const binding = loadBinding();

module.exports = {
  version: binding.version,
  versionInt: binding.versionInt,
  versionAbi: binding.versionAbi,
  data: createData(binding.data),
  client: createClient(binding.client),
  server: binding.server,
};
