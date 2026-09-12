'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/reference/benchmark-engine.js','utf8');
function environment(step){
  let clock=0;const messages=[];
  const env={performance:{now:()=>clock+=step},self:{postMessage:message=>messages.push(structuredClone(message))}};
  vm.createContext(env);vm.runInContext(source,env);return{env,messages};
}
function validPartial(graph,snapshot){
  assert.equal(snapshot.colors.length,graph.n);
  for(const color of snapshot.colors)assert.ok(Number.isInteger(color)&&color>=-1&&color<4);
  graph.adj.forEach((neighbors,u)=>neighbors.forEach(v=>{
    if(snapshot.colors[u]>=0&&snapshot.colors[v]>=0)assert.notEqual(snapshot.colors[u],snapshot.colors[v]);
  }));
  assert.doesNotThrow(()=>structuredClone(snapshot));
}
// Advance a deterministic clock at checkpoints to exercise 100-second behavior
// without waiting 100 seconds. Never accelerate the production/browser clock.
const {env,messages}=environment(250);
env.self.onmessage({data:{type:'generate',side:64,rows:32,seed:817,requestId:1}});
env.self.onmessage({data:{type:'solve',method:'dsatur',requestId:2}});
const graph=env.makeMap(64,817,32),progress=messages.filter(m=>m.type==='progress'),result=messages.at(-1);
assert.equal(result.type,'solved');assert.equal(result.status,'timeout');assert.equal(result.colors,null);
assert.ok(result.solverMs>=99500&&result.solverMs<102000,'Default budget must be 100 seconds.');
assert.ok(progress.length>=19,'Periodic snapshots stopped before the deadline.');
for(let i=0;i<progress.length;i++){
  const snapshot=progress[i];assert.equal(snapshot.requestId,2);assert.equal(snapshot.method,'dsatur');
  validPartial(graph,snapshot);
  if(i&&i<progress.length-1)assert.ok(snapshot.solverMs-progress[i-1].solverMs>=5000);
}
assert.ok(progress.some(p=>p.colors.includes(-1)&&p.colors.some(c=>c>=0)),'No visible partial coloring.');
assert.ok(progress.at(-1).solverMs>progress[0].solverMs);

// Exercise restoration and timeouts with frequent checkpoints. Snapshot copies
// must remain valid after later backtracking and complete Kempe component swaps.
let restorationSnapshots=0;
for(const budget of [10000,20000,40000,1000000]){
  const {env:reduction}=environment(2500),g=reduction.makeMap(32,817),snapshots=[];
  const solved=reduction.execute(g,'reduction',budget,p=>snapshots.push(p));
  for(const snapshot of snapshots){validPartial(g,snapshot);if(snapshot.phase==='Restoring')restorationSnapshots++;}
  if(budget===1000000){assert.equal(solved.status,'complete');assert.ok(solved.stats.swaps>0);}
}
assert.ok(restorationSnapshots>0,'No restoration snapshots were checked.');
console.log(JSON.stringify({defaultBudgetMs:100000,dsaturSnapshots:progress.length,restorationSnapshots,checks:['5-second cadence','request correlation','proper partial colorings','snapshot copies','safe Kempe updates','terminal timeout snapshot','serializable statistics']}));
