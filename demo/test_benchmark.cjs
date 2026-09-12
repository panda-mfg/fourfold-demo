'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/benchmark-engine.js','utf8');
const env={performance,self:{postMessage(){}}};vm.createContext(env);vm.runInContext(source,env);
function valid(graph,result){
  assert.equal(result.status,'complete',result.message);assert.equal(result.colors.length,graph.adj.length);
  for(const c of result.colors)assert.ok(c>=0&&c<4);
  graph.adj.forEach((neighbors,u)=>neighbors.forEach(v=>assert.notEqual(result.colors[u],result.colors[v])));
}
function graph(n,edges){const adj=Array.from({length:n},()=>[]);for(const[u,v]of edges){adj[u].push(v);adj[v].push(u);}return{adj};}
const fixtures=[];
fixtures.push(['triangle',graph(3,[[0,1],[1,2],[2,0]])]);
fixtures.push(['tetrahedron',graph(4,[[0,1],[0,2],[0,3],[1,2],[1,3],[2,3]])]);
fixtures.push(['shared-ring octahedron',graph(6,[[2,3],[3,4],[4,5],[5,2],...[0,1].flatMap(u=>[2,3,4,5].map(v=>[u,v]))])]);
const ico=[];for(let i=0;i<5;i++){ico.push([0,i+1],[11,i+6],[i+1,(i+1)%5+1],[i+6,(i+1)%5+6],[i+1,i+6],[i+1,(i+4)%5+6]);}
fixtures.push(['icosahedron search core',graph(12,ico)]);
for(const[name,g]of fixtures)for(const method of ['dsatur','reduction']){
  const result=env.execute(g,method,1000);valid(g,result);
  if(name==='icosahedron search core'&&method==='reduction')assert.equal(result.stats.coreVertices,12);
}
const k5=graph(5,Array.from({length:5},(_,i)=>Array.from({length:4-i},(_,j)=>[i,i+j+1])).flat());
for(const method of ['dsatur','reduction'])assert.equal(env.execute(k5,method,1000).status,'unsupported');

// Verify the optimized local clipping stencil against EVERY seed, not the
// production neighborhood test. All polygon vertices must obey every bisector.
let generated=0,sharedBorders=0,swaps=0;
for(const [side,rows] of [[8,8],[16,16],[32,32],[64,32]])for(let seed=1;seed<=(side===rows?12:1);seed++){
  const g=env.makeMap(side,seed*97,rows),r=env.rng(seed*97),sites=[];
  assert.equal(g.n,side*rows);assert.equal(g.adj.length,side*rows);assert.equal(g.offsets.length,side*rows+1);
  for(let y=0;y<rows;y++)for(let x=0;x<side;x++)sites.push([x+.5+(r()-.5)*.5,y+.5+(r()-.5)*.5]);
  let area=0;const geometricEdges=new Map();
  for(let u=0;u<g.n;u++){
    const start=g.offsets[u],end=g.offsets[u+1];let double=0;
    for(let j=start;j<end;j++){
      const k=j+1<end?j+1:start,a=[g.xy[2*j],g.xy[2*j+1]],b=[g.xy[2*k],g.xy[2*k+1]];
      double+=a[0]*b[1]-b[0]*a[1];
      if(Math.hypot(a[0]-b[0],a[1]-b[1])>1e-9){
        const key=[a.map(x=>x.toFixed(8)).join(','),b.map(x=>x.toFixed(8)).join(',')].sort().join('|');
        if(!geometricEdges.has(key))geometricEdges.set(key,[]);geometricEdges.get(key).push(u);
      }
      if(side<=16||side!==rows){
        const x=a[0]*side,y=a[1]*rows,d=(x-sites[u][0])**2+(y-sites[u][1])**2;
        for(const p of sites)assert.ok(d<=(x-p[0])**2+(y-p[1])**2+1e-7,'Stencil missed a nearer seed.');
      }
    }
    assert.ok(double>0);area+=double/2;
  }
  assert.ok(Math.abs(area-1)<1e-9);
  const reconstructed=new Set();
  for(const owners of geometricEdges.values()){
    assert.ok(owners.length<=2);
    if(owners.length===2)reconstructed.add(owners.sort((a,b)=>a-b).join(','));
  }
  const declared=new Set();for(let i=0;i<g.edges.length;i+=2)declared.add(`${g.edges[i]},${g.edges[i+1]}`);
  assert.deepEqual(reconstructed,declared,'Border labels disagree with independent geometry.');
  const result=env.execute(g,'reduction',3000);valid(g,result);swaps+=result.stats.swaps;generated++;sharedBorders+=declared.size;
  if(side===8)valid(g,env.execute(g,'dsatur',1000));
}
assert.ok(swaps>0,'Kempe restoration was never exercised.');

// Check every intermediate Kempe recoloring on one nontrivial fixture. This
// assertion is injected only into the test copy and never into timed code.
const audited=source.replace('free=1<<ca;ctx.swaps++;found=true;',`for(let x=0;x<adj.length;x++)for(const y of adj[x]){if(colors[x]>=0&&colors[x]===colors[y])throw new Error('Improper intermediate Kempe state.');}free=1<<ca;ctx.swaps++;found=true;`);
const audit={performance,self:{postMessage(){}}};vm.createContext(audit);vm.runInContext(audited,audit);
const sample=audit.makeMap(16,817);valid(sample,audit.execute(sample,'reduction',3000));
let clock=0;const limited={performance:{now:()=>{clock+=50;return clock;}},self:{postMessage(){}}};vm.createContext(limited);vm.runInContext(source,limited);
assert.equal(limited.execute(fixtures[1][1],'dsatur',1).status,'timeout');
const summary={fixtureAlgorithmsChecked:fixtures.length*2,generatedMapsChecked:generated,geometricSharedBordersChecked:sharedBorders,kempeSwapsExercised:swaps,checks:['all-site clipping constraints','polygon area coverage','independent border reconstruction','proper completed colorings','nonempty DSATUR core','intermediate Kempe invariants','unsupported K5','explicit timeout']};
console.log(JSON.stringify(summary,null,2));
