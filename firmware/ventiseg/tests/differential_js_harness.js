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
const mode = process.argv[3] || 'parse';
const input = JSON.parse(fs.readFileSync(0, 'utf8'));

function asLine(bytes) {
  // Cada byte vira um code unit; feed() barra na validacao de codificacao.
  let line = '';
  for (const byte of bytes) line += String.fromCharCode(byte);
  return line;
}

let results;
if (mode === 'session') {
  results = input.map((script) => {
    const s = core.createState();
    const trace = [];
    for (const step of script.steps) {
      let result;
      if (step.op === 'feed') {
        // null (JSON) significa "sem atraso de fila" na API Python/C++.
        const now = step.now === null ? undefined : step.now;
        result = core.feed(s, asLine(step.datagram), now, step.rx);
      } else if (step.op === 'poll') {
        result = core.checkWatchdog(s, step.now);
      } else {
        throw new Error('operacao desconhecida: ' + step.op);
      }
      trace.push({
        result,
        telemetry: core.telemetry(s),
        mode: s.mode,
        fault: s.fault,
        seq: s.seq,
        axis: s.axis,
        moving: s.moving,
        linkLost: s.linkLost,
      });
    }
    return { name: script.name, trace };
  });
} else {
  results = input.map((item) => ({
    name: item.name,
    decision: core.parseDecision(
      asLine(item.datagram), item.rx_ms, item.now_ms, item.last_seq
    ),
  }));
}

process.stdout.write(JSON.stringify(results));
