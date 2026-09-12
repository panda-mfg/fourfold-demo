'use strict';
const assert = require('node:assert/strict'), fs = require('node:fs'), {Worker} = require('node:worker_threads');
const {FourfoldWorkerPool} = require('./worker-pool.js');
const bytes = fs.readFileSync(__dirname + '/../assets/fourfold_engine.wasm');
const source = ['map-generator.js', 'wasm-adapter.js', 'benchmark-engine.js']
  .map(name => fs.readFileSync(__dirname + '/' + name, 'utf8')).join('\n')
  .replace('__FOURFOLD_WASM_BASE64__', bytes.toString('base64'));
const prefix = `const {parentPort}=require('node:worker_threads');
const self={postMessage:data=>parentPort.postMessage(data)};
parentPort.on('message',data=>self.onmessage({data}));\n`;
let created = 0, stopped = 0;
const running = new Set(), terminations = [];
function factory() {
  const worker = new Worker(prefix + source, {eval: true});
  created++; running.add(worker);
  let killed = false;
  const facade = {postMessage: data => worker.postMessage(data), terminate() {
    if (!killed) { killed = true; stopped++; running.delete(worker); terminations.push(worker.terminate()); }
  }};
  worker.on('message', data => facade.onmessage?.({data}));
  worker.on('error', error => facade.onerror?.(error));
  return facade;
}
function valid(graph, colors) {
  assert.equal(colors.length, graph.n);
  for (const c of colors) assert.ok(c >= 0 && c < 4);
  for (let i = 0; i < graph.edges.length; i += 2) assert.notEqual(colors[graph.edges[i]], colors[graph.edges[i + 1]]);
}
async function main() {
  const records = [];
  for (const threads of [1, 2, 4, 8]) {
    const pool = new FourfoldWorkerPool(source, threads, factory);
    try {
      const graph = await pool.generate({side: threads === 1 ? 32 : 64, rows: 32, seed: 817});
      for (const method of ['dsatur', 'reduction', 'dsatur']) {
        const result = await pool.solve(method, 5000);
        assert.equal(result.status, 'complete'); valid(graph, result.colors);
        assert.equal(result.threads, threads);
        assert.equal(result.attempts.length, threads);
        assert.ok(result.winningWorker >= 1 && result.winningWorker <= threads);
        assert.ok(result.solverMs >= result.workerMs);
        assert.ok(result.attempts.every(a => a.status !== 'running'));
        records.push({threads, method, ms: result.solverMs, winner: result.winningWorker});
      }
      // Deadline is shared across the workers; no false completion or colors.
      await pool.generate({side: 128, rows: 128, seed: 817});
      const timed = await pool.solve('dsatur', 1);
      assert.equal(timed.status, 'timeout'); assert.equal(timed.colors, null);
      await pool.prepare();
      const active = pool.solve('dsatur', 100000);
      await assert.rejects(pool.solve('reduction'), /already running/);
      await new Promise(resolve => setTimeout(resolve, 5));
      pool.close(); await assert.rejects(active, /Cancelled/);
    } finally { pool.close(); }
  }
  // Crashed or incorrect workers cannot discard or replace a valid winner.
  for (const fault of ['crash', 'invalid coloring']) {
  let ordinal = 0;
  const pool = new FourfoldWorkerPool(source, 2, () => {
    const worker = factory(), index = ordinal++, send = worker.postMessage;
    worker.postMessage = data => {
      if (index === 0 && data.type === 'solve') queueMicrotask(() => {
        if (fault === 'crash') worker.onerror({message: 'Injected worker failure'});
        else worker.onmessage({data:{requestId:data.requestId,type:'solved',status:'complete',colors:new Int8Array(64)}});
      });
      else send(data);
    };
    return worker;
  });
  try {
    const graph = await pool.generate({side: 8, rows: 8, seed: 817});
    const result = await pool.solve('dsatur', 5000);
    assert.equal(result.status, 'complete'); valid(graph, result.colors);
    assert.equal(result.winningWorker, 2); assert.equal(result.attempts[0].status, fault === 'crash' ? 'error' : 'invalid');
  } finally { pool.close(); }
  }
  await Promise.all(terminations);
  assert.equal(running.size, 0); assert.equal(created, stopped);
  console.log(JSON.stringify({realWorkerThreads: created, leakedWorkers: running.size, records,
    checks: ['1/2/4/8 workers', 'valid winners', 'loser cancellation and restart', 'shared deadline', 'stop', 'overlap rejected', 'worker failure isolation']}));
}
main().catch(async error => { for (const worker of running) await worker.terminate(); console.error(error); process.exitCode = 1; });
