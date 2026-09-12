'use strict';
// A portfolio uses private WASM memories and real Web Workers. No shared-memory
// WASM, special HTTP headers, or unsafe concurrent Kempe swaps are required.
class FourfoldWorkerPool {
  constructor(source, count, factory = null) {
    if (!Number.isInteger(count) || count < 1 || count > 8) throw new Error('Use 1 to 8 workers.');
    this.count = count;
    this.url = factory ? null : URL.createObjectURL(new Blob([source], {type: 'application/javascript'}));
    this.factory = factory || (() => new Worker(this.url));
    this.slots = Array(count).fill(null);
    this.serial = 0;
    this.closed = false;
    this.graph = null;
    this.abort = null;
    this.busy = false;
  }
  slot(index) {
    if (this.closed) throw new Error('Cancelled.');
    if (this.slots[index]) return this.slots[index];
    const slot = {worker: this.factory(), pending: new Map(), ready: false};
    this.slots[index] = slot;
    slot.worker.onmessage = ({data}) => {
      const pending = slot.pending.get(data.requestId);
      if (!pending) return;
      if (data.type === 'progress') { pending.progress?.(data); return; }
      slot.pending.delete(data.requestId);
      clearTimeout(pending.timer);
      if (data.type === 'error') pending.reject(new Error(data.message));
      else pending.resolve(data);
    };
    slot.worker.onerror = error => this.kill(index, new Error(error.message || 'Worker failed.'));
    return slot;
  }
  kill(index, error = new Error('Cancelled.')) {
    const slot = this.slots[index];
    if (!slot) return;
    this.slots[index] = null;
    slot.worker.terminate();
    for (const pending of slot.pending.values()) { clearTimeout(pending.timer); pending.reject(error); }
    slot.pending.clear();
  }
  request(index, data, limit = 20000, progress = null) {
    const slot = this.slot(index);
    return new Promise((resolve, reject) => {
      const requestId = ++this.serial;
      const timer = setTimeout(() => this.kill(index, new Error('Worker safety limit reached.')), limit);
      slot.pending.set(requestId, {resolve, reject, timer, progress});
      try { slot.worker.postMessage({...data, requestId}); }
      catch (error) { this.kill(index, error); }
    });
  }
  async generate(data) {
    if (this.busy) throw new Error('A trial is already running.');
    for (const slot of this.slots) if (slot) slot.ready = false;
    this.graph = await this.request(0, {...data, type: 'generate'});
    if (this.closed) throw new Error('Cancelled.');
    this.slot(0).ready = true;
    return this.graph;
  }
  async prepare() {
    if (!this.graph) throw new Error('Generate a map first.');
    await Promise.all(Array.from({length: this.count}, async (_, index) => {
      const slot = this.slot(index);
      if (slot.ready) return;
      await this.request(index, {type: 'prepare', n: this.graph.n, edges: this.graph.edges});
      if (this.closed) throw new Error('Cancelled.');
      slot.ready = true;
    }));
  }
  valid(colors) {
    if (!colors || colors.length !== this.graph.n) return false;
    for (const color of colors) if (!Number.isInteger(color) || color < 0 || color > 3) return false;
    const edges = this.graph.edges;
    for (let i = 0; i < edges.length; i += 2) if (colors[edges[i]] === colors[edges[i + 1]]) return false;
    return true;
  }
  async solve(method, budgetMs = 100000, progress = null) {
    if (this.busy) throw new Error('A trial is already running.');
    if (!['dsatur', 'reduction'].includes(method) || !Number.isFinite(budgetMs) || budgetMs <= 0 || budgetMs > 100000) throw new Error('Invalid trial.');
    this.busy = true;
    try {
      await this.prepare(); // Creation, compilation, graph import, and warm-up are untimed.
      if (this.closed) throw new Error('Cancelled.');
      return await new Promise((resolve, reject) => {
        const start = performance.now(), deadlineEpoch = performance.timeOrigin + start + budgetMs;
        const attempts = Array.from({length: this.count}, (_, i) => ({worker: i + 1, variant: i, status: 'running'}));
        let settled = false, last = null, snapshots = 0;
        const finish = (result, winner = null) => {
          if (settled) return;
          settled = true; clearTimeout(timer); this.abort = null;
          attempts.forEach((attempt, index) => {
            if (attempt.status === 'running') { attempt.status = winner === null ? 'timeout' : 'cancelled'; this.kill(index); }
          });
          resolve({...result, method, backend: 'rust-wasm', threads: this.count,
            strategy: this.count === 1 ? 'single' : 'search-order portfolio', winningWorker: winner,
            workerMs: winner === null ? null : result.solverMs ?? null, solverMs: performance.now() - start, attempts,
            stats: result.stats ? {...result.stats, progressUpdates: snapshots} : null});
        };
        const timer = setTimeout(() => finish({status: 'timeout', colors: null, message: 'Shared trial deadline reached.', stats: last?.stats}), budgetMs);
        this.abort = () => {
          if (settled) return;
          settled = true; clearTimeout(timer); this.abort = null; reject(new Error('Cancelled.'));
        };
        for (let index = 0; index < this.count; index++) {
          this.request(index, {type: 'solve', method, budgetMs, deadlineEpoch, variant: index}, budgetMs + 5000, data => {
            if (settled || index !== attempts.findIndex(a => a.status === 'running')) return;
            snapshots++; last = data;
            progress?.({...data, solverMs: performance.now() - start, worker: index + 1, threads: this.count});
          }).then(result => {
            if (settled) return;
            if (result.status === 'complete' && !this.valid(result.colors)) result = {...result, status: 'invalid', colors: null, message: 'Worker coloring failed independent validation.'};
            attempts[index] = {...attempts[index], status: result.status, workerMs: result.solverMs ?? null};
            if (result.status === 'complete' && performance.now() - start <= budgetMs) { finish(result, index + 1); return; }
            if (result.status === 'complete') attempts[index].status = 'timeout';
            last = result;
            if (attempts.every(a => a.status !== 'running')) {
              const status = attempts.some(a => ['error', 'invalid'].includes(a.status)) ? 'error' : attempts.some(a => a.status === 'timeout') ? 'timeout' : 'unsupported';
              finish({...last, status, colors: null, message: status === 'timeout' ? 'Shared trial deadline reached.' : 'No worker returned a valid coloring.'});
            }
          }).catch(error => {
            if (settled) return;
            attempts[index].status = 'error';
            attempts[index].message = error.message;
            if (attempts.every(a => a.status !== 'running')) finish({status: 'error', colors: null, message: error.message});
          });
        }
      });
    } finally { this.busy = false; }
  }
  close() {
    this.closed = true;
    this.abort?.();
    for (let i = 0; i < this.count; i++) this.kill(i);
    if (this.url) URL.revokeObjectURL(this.url);
  }
}
if (typeof module !== 'undefined') module.exports = {FourfoldWorkerPool};
