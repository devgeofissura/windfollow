/*
 * Harness de testes diferenciais do nucleo JS do ventiseg.
 *
 * Le um JSON com casos da entrada padrao e devolve as decisoes do mesmo
 * feed() usado pelo simulador. Nenhum DOM, nenhuma rede, nenhuma placa.
 *
 * Uso: node differential_js_harness.js <caminho para simulator/js/app.js>
 */
'use strict';

const fs = require('fs');
const vm = require('vm');

const appPath = process.argv[2];
if (!appPath) {
  process.stderr.write('uso: node differential_js_harness.js <app.js>\n');
  process.exit(2);
}

const sandbox = { console };
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync(appPath, 'utf8'), sandbox,
                { filename: 'app.js' });
const core = sandbox.VentisegCore;
const cases = JSON.parse(fs.readFileSync(0, 'utf8'));

const results = cases.map((item) => {
  // Cada byte vira um code unit; feed() barra na validacao de codificacao.
  let line = '';
  for (const byte of item.datagram) {
    line += String.fromCharCode(byte);
  }
  return {
    name: item.name,
    decision: core.parseDecision(
      line, item.rx_ms, item.now_ms, item.last_seq
    ),
  };
});

process.stdout.write(JSON.stringify(results));
