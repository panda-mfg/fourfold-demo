'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const {createWasmEngine}=require('./wasm-adapter.js'),{makeMap}=require('./map-generator.js');
const bytes=fs.readFileSync(__dirname+'/../assets/fourfold_engine.wasm');
const reference={performance,self:{postMessage(){}}};vm.createContext(reference);
vm.runInContext(fs.readFileSync(__dirname+'/reference/benchmark-engine.js','utf8'),reference);
function graph(n,pairs){const adj=Array.from({length:n},()=>[]);for(const[u,v]of pairs){adj[u].push(v);adj[v].push(u);}adj.forEach(a=>a.sort((x,y)=>x-y));return{n,adj};}
function valid(g,colors,partial=false){
 assert.equal(colors.length,g.n);
 for(const c of colors)assert.ok(c>= (partial?-1:0)&&c<4);
 g.adj.forEach((neighbors,u)=>neighbors.forEach(v=>{if(colors[u]>=0&&colors[v]>=0)assert.notEqual(colors[u],colors[v]);}));
}
(async()=>{
 const engine=await createWasmEngine(bytes);let checks=0;
 const ico=[];for(let i=0;i<5;i++)ico.push([0,i+1],[11,i+6],[i+1,(i+1)%5+1],[i+6,(i+1)%5+6],[i+1,i+6],[i+1,(i+4)%5+6]);
 const fixtures=[graph(1,[]),graph(3,[[0,1],[1,2],[2,0]]),graph(4,[[0,1],[0,2],[0,3],[1,2],[1,3],[2,3]]),graph(6,[[2,3],[3,4],[4,5],[5,2],...[0,1].flatMap(u=>[2,3,4,5].map(v=>[u,v]))]),graph(12,ico)];
 for(const g of [...fixtures,...[8,16,32].flatMap(side=>[97,194,817].map(seed=>makeMap(side,seed)))]){
  engine.prepare(g);
  for(const method of ['dsatur','reduction']){
   const expected=reference.execute(g,method,2000),actual=engine.execute(method,2000);
   if(expected.status==='timeout'){assert.ok(['complete','timeout'].includes(actual.status));if(actual.status==='complete')valid(g,actual.colors);continue;}
   assert.equal(actual.status,expected.status);valid(g,actual.colors);
   assert.deepEqual(Array.from(actual.colors),Array.from(expected.colors));
   for(const key of ['decisions','backtracks','batches','removed','swaps','kempeVisits','coreVertices'])assert.equal(actual.stats[key],expected.stats[key],key);
   checks++;
  }
 }
 for(const [side,rows] of [[64,32],[64,64],[128,128]]){
  const g=makeMap(side,817,rows);engine.prepare(g);const result=engine.execute('reduction');valid(g,result.colors);checks++;
 }
 const k5=graph(5,Array.from({length:5},(_,u)=>Array.from({length:4-u},(_,i)=>[u,u+i+1])).flat());
 let variants=0;
 for(let variant=0;variant<8;variant++){
  for(const g of [...fixtures,makeMap(16,97)]){
   engine.prepare(g);
   for(const method of ['dsatur','reduction']){const result=engine.execute(method,2000,null,variant);assert.equal(result.status,'complete');valid(g,result.colors);variants++;}
  }
  engine.prepare(k5);for(const method of ['dsatur','reduction'])assert.equal(engine.execute(method,2000,null,variant).status,'unsupported');
 }
 engine.prepare(k5);for(const method of ['dsatur','reduction'])assert.equal(engine.execute(method).status,'unsupported');
 assert.throws(()=>engine.prepare({n:2,edges:[0,0]}));assert.throws(()=>engine.prepare({n:2,edges:[0,2]}));
 const g=makeMap(64,817,32);let clock=0;
 const limited=await createWasmEngine(bytes,()=>clock+=250);limited.prepare(g);const snapshots=[];
 const timeout=limited.execute('dsatur',100000,p=>snapshots.push(p));
 assert.equal(timeout.status,'timeout');assert.ok(timeout.solverMs>=100000);assert.ok(snapshots.length>=19);
 for(let i=0;i<snapshots.length;i++){valid(g,snapshots[i].colors,true);if(i&&i<snapshots.length-1)assert.ok(snapshots[i].solverMs-snapshots[i-1].solverMs>=5000);}
 let restoration=0;
 for(const budget of [10000,20000,40000,100000]){
  let timer=0;const slow=await createWasmEngine(bytes,()=>timer+=2500),map=makeMap(32,817);slow.prepare(map);
  slow.execute('reduction',budget,p=>{valid(map,p.colors,true);if(p.phase==='Restoring')restoration++;});
 }
 assert.ok(restoration>0);
 console.log(JSON.stringify({backend:'rust-wasm',differentialAndLargeMapChecks:checks,variantChecks:variants,timeoutSnapshots:snapshots.length,restorationSnapshots:restoration,invalidInputs:'rejected'}));
})().catch(error=>{console.error(error);process.exitCode=1;});
