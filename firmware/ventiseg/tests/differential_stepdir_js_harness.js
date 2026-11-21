// Harness diferencial da camada STEP/DIR do simulador.
// Le um JSON com a lista de operacoes e imprime um JSON com os snapshots.
const fs = require('fs');
const vm = require('vm');

const sandbox = { console };
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync(process.argv[2], 'utf8'), sandbox,
                { filename: 'app.js' });
const C = sandbox.VentisegCore;

let raw = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => { raw += chunk; });
process.stdin.on('end', () => {
  const ops = JSON.parse(raw);
  const driver = C.createStepDir();
  const rows = [];
  for (let index = 0; index < ops.length; index += 1) {
    const [kind, value] = ops[index];
    let reply = '';
    if (kind === 'update') C.stepDirUpdate(driver, value);
    else if (kind === 'target') C.setStepDirTarget(driver, value);
    else if (kind === 'cfg') reply = C.stepDirConfig(driver, value);
    const row = { index, op: kind, reply };
    Object.assign(row, C.stepDirSnapshot(driver));
    rows.push(row);
  }
  process.stdout.write(JSON.stringify(rows));
});
