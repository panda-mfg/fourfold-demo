'use strict';
const fs=require('node:fs'),assert=require('node:assert/strict');
const {createWasmEngine}=require('./wasm-adapter.js'),{makeMap}=require('./map-generator.js');
const bytes=fs.readFileSync(__dirname+'/../assets/fourfold_engine.wasm');
function validate(g,colors,partial=false){
  assert.equal(colors.length,g.n);
  for(const c of colors)assert.ok(Number.isInteger(c)&&c>=(partial?-1:0)&&c<4);
  for(let i=0;i<g.edges.length;i+=2){const a=colors[g.edges[i]],b=colors[g.edges[i+1]];if(a>=0&&b>=0)assert.notEqual(a,b);}
}
(async()=>{
  const engine=await createWasmEngine(bytes);let variants=0;const records=[];
  for(const seed of [97,194,817])for(const side of [8,16]){
    const g=makeMap(side,seed);engine.prepare(g);
    for(let variant=0;variant<8;variant++){
      const r=engine.execute('rsst',10000,null,variant);assert.equal(r.status,'complete',r.message);validate(g,r.colors);
      assert.equal(r.stats.decisions,0);assert.equal(r.stats.backtracks,0);variants++;
    }
  }
  for(const [side,rows] of [[32,32],[64,32],[64,64],[128,128]]){
    const g=makeMap(side,817,rows);engine.prepare(g);const r=engine.execute('rsst',100000);
    if(g.n<=2048){assert.equal(r.status,'complete',r.message);validate(g,r.colors);}
    else {assert.ok(['complete','resource-limit','timeout'].includes(r.status),r.message);if(r.colors)validate(g,r.colors);}
    if(g.n===2048){assert.ok(r.rsst.cReductions>0);assert.ok(r.rsst.dReductions>0);assert.ok(r.rsst.boundaryStates>r.rsst.configurations);}
    const {colors,...record}=r;records.push({n:g.n,fingerprint:g.fingerprint,...record});
  }
  let clock=0;const slow=await createWasmEngine(bytes,()=>clock+=20),g=makeMap(32,817);slow.prepare(g);const snapshots=[];
  const complete=slow.execute('rsst',100000,p=>{validate(g,p.colors,true);snapshots.push(p);});
  assert.equal(complete.status,'complete',complete.message);assert.ok(snapshots.length>=2);assert.ok(snapshots.some(p=>p.colors.some(c=>c>=0)));
  for(let i=1;i<snapshots.length;i++)assert.ok(snapshots[i].solverMs-snapshots[i-1].solverMs>=5000);
  clock=0;const limited=await createWasmEngine(bytes,()=>clock+=20);limited.prepare(g);
  const timeout=limited.execute('rsst',10000,p=>validate(g,p.colors,true));assert.equal(timeout.status,'timeout');assert.equal(timeout.colors,null);
  engine.prepare({n:g.n,edges:g.edges});assert.equal(engine.execute('rsst').status,'error');
  const bad=makeMap(8,1),u=bad.rotation.findIndex(row=>row.length>=4);[bad.rotation[u][0],bad.rotation[u][1]]=[bad.rotation[u][1],bad.rotation[u][0]];
  assert.throws(()=>engine.prepare(bad),/planar embedding/);
  bad.rotation[0][0]=0.5;assert.throws(()=>engine.prepare(bad),/rotation neighbor/);
  const result={backend:'rust-wasm',variantFixtures:variants,safeProgressSnapshots:snapshots.length,records};
  console.log(JSON.stringify(result,null,2));
})().catch(e=>{console.error(e);process.exitCode=1});
