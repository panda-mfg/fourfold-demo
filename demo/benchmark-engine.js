/* Fourfold benchmark worker. Both methods use the same prepared adjacency lists.
 * The reduction method is a limited prototype, NOT the paper's full algorithm.
 * All solver allocations, reductions, core search, and restoration are timed.
 * Periodic progress copying/reporting is included; map construction, warm-up,
 * final transfer, independent validation, and painting are not.
 */
'use strict';
let current = null;
const now = () => performance.now();
class Stop extends Error { constructor(status,message){super(message);this.status=status;} }
function rng(seed){let a=seed>>>0;return()=>{a=(a+0x6D2B79F5)>>>0;let t=a;t=Math.imul(t^(t>>>15),t|1);t^=t+Math.imul(t^(t>>>7),t|61);return((t^(t>>>14))>>>0)/4294967296;};}

function makeMap(side,seed,rows=side){
  if(![8,16,32,64,128].includes(side)||(rows!==side&&!(side===64&&rows===32)))throw new Error('Unsupported map size.');
  const started=now(),n=side*rows,random=rng(seed),points=new Float64Array(n*2);
  for(let r=0;r<rows;r++)for(let c=0;c<side;c++){const i=r*side+c;points[2*i]=c+.5+(random()-.5)*.5;points[2*i+1]=r+.5+(random()-.5)*.5;}
  const xy=[],offsets=new Uint32Array(n+1),edges=[],adj=Array.from({length:n},()=>[]);
  // Seeds are within .25 of each unit-square center. Every point has a seed
  // within sqrt(2)*.75. A seed >=3 grid columns/rows away is >2*that radius
  // from this seed, so it cannot cut this Voronoi cell. The 5x5 stencil is exact.
  for(let i=0;i<n;i++){
    const r=Math.floor(i/side),c=i%side,px=points[2*i],py=points[2*i+1];
    let poly=[{x:0,y:0,owner:-1},{x:side,y:0,owner:-1},{x:side,y:rows,owner:-1},{x:0,y:rows,owner:-1}];
    for(let rr=Math.max(0,r-2);rr<=Math.min(rows-1,r+2);rr++)for(let cc=Math.max(0,c-2);cc<=Math.min(side-1,c+2);cc++){
      const j=rr*side+cc;if(i===j)continue;
      const qx=points[2*j],qy=points[2*j+1],nx=2*(qx-px),ny=2*(qy-py),k=qx*qx+qy*qy-px*px-py*py,next=[];
      // owner labels the incoming edge, preserving exact border provenance.
      for(let z=0;z<poly.length;z++){
        const a=poly[z],b=poly[(z+1)%poly.length],fa=nx*a.x+ny*a.y-k,fb=nx*b.x+ny*b.y-k;
        if(fb<=0){
          if(fa>0){const t=fa/(fa-fb);next.push({x:a.x+t*(b.x-a.x),y:a.y+t*(b.y-a.y),owner:j});}
          next.push(b);
        }else if(fa<=0){const t=fa/(fa-fb);next.push({x:a.x+t*(b.x-a.x),y:a.y+t*(b.y-a.y),owner:b.owner});}
      }
      poly=next;
    }
    if(poly.length<3)throw new Error('Empty map region.');
    offsets[i]=xy.length/2;
    const seen=new Set();
    for(let z=0;z<poly.length;z++){
      const b=poly[z],a=poly[(z+poly.length-1)%poly.length];xy.push(b.x/side,b.y/rows);
      if(b.owner>i&&!seen.has(b.owner)&&Math.hypot(b.x-a.x,b.y-a.y)>1e-10){seen.add(b.owner);edges.push(i,b.owner);adj[i].push(b.owner);adj[b.owner].push(i);}
    }
  }
  offsets[n]=xy.length/2;adj.forEach(a=>a.sort((x,y)=>x-y));
  let hash=2166136261;for(const x of [n,seed,...edges]){hash^=x;hash=Math.imul(hash,16777619)>>>0;}
  return{n,side,rows,seed,adj,edges:new Uint32Array(edges),xy:new Float64Array(xy),offsets,fingerprint:hash.toString(16).padStart(8,'0'),generationMs:now()-started};
}
function context(budgetMs){
  return{deadline:now()+budgetMs,probes:0,decisions:0,backtracks:0,batches:0,removed:0,swaps:0,kempeVisits:0,coreVertices:0,reductionMs:0,coreMs:0,restorationMs:0,progressUpdates:0};
}
// Keep callbacks and mutable color buffers out of the serializable statistics.
const liveRuns=new WeakMap();
function progressState(ctx,colors,phase){const live=liveRuns.get(ctx);if(live){live.colors=colors;live.phase=phase;}}
function reportProgress(ctx,stamp,force=false){
  const live=liveRuns.get(ctx);
  if(!live||!live.safe||!live.colors||(!force&&stamp<live.nextAt))return;
  live.nextAt=stamp+5000;ctx.progressUpdates++;
  live.report({colors:live.colors.slice(),solverMs:stamp-live.started,phase:live.phase,stats:{...ctx}});
}
function check(ctx,force=false){
  if(force||(ctx.probes&4095)===0){
    const stamp=now();if(stamp>ctx.deadline)throw new Stop('timeout','Time limit reached.');
    reportProgress(ctx,stamp);
  }
}
const bits=new Uint8Array([0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4]);
function dsatur(adj,vertices,colors,ctx){
  const n=adj.length,counts=new Uint32Array(n*4),mask=new Uint8Array(n),stack=[];
  // Iterative backtracking avoids call-stack limits on large maps. Saturation
  // counts are maintained incrementally; selection uses the original tie rule.
  const assign=(u,c,add)=>{
    colors[u]=add?c:-1;
    for(const v of adj[u]){
      const index=v*4+c;
      if(add){if(counts[index]++===0)mask[v]|=1<<c;}
      else if(--counts[index]===0)mask[v]&=~(1<<c);
      ctx.probes++;check(ctx);
    }
  };
  let forward=true;
  for(;;){
    if(forward){
      let best=-1,saturation=-1,degree=-1;
      for(const u of vertices){
        ctx.probes++;check(ctx);
        if(colors[u]<0){const s=bits[mask[u]],d=adj[u].length;if(s>saturation||(s===saturation&&d>degree)){best=u;saturation=s;degree=d;}}
      }
      if(best<0)return;
      stack.push({u:best,left:15&~mask[best],assigned:-1});
    }
    forward=false;
    while(stack.length){
      const f=stack[stack.length-1];
      if(f.assigned>=0){assign(f.u,f.assigned,false);f.assigned=-1;}
      if(f.left){const bit=f.left&-f.left;f.left^=bit;const c=bit===1?0:bit===2?1:bit===4?2:3;f.assigned=c;assign(f.u,c,true);ctx.decisions++;forward=true;break;}
      stack.pop();ctx.backtracks++;
    }
    if(!forward)throw new Stop('unsupported','Search exhausted without a four-coloring.');
  }
}
function classic(adj,ctx){
  const colors=new Int8Array(adj.length);colors.fill(-1);ctx.coreVertices=adj.length;
  progressState(ctx,colors,'Searching');
  const vertices=Array.from({length:adj.length},(_,i)=>i),started=now();
  dsatur(adj,vertices,colors,ctx);ctx.coreMs=now()-started;return colors;
}
function reduction(adj,ctx){
  const n=adj.length,colors=new Int8Array(n);colors.fill(-1);
  progressState(ctx,colors,'Reducing');const live=liveRuns.get(ctx);
  const active=new Uint8Array(n);active.fill(1);
  const degree=Uint32Array.from(adj,a=>a.length),blocked=new Uint32Array(n),queued=new Uint32Array(n),removed=[];
  let eligible=[];for(let u=0;u<n;u++)if(degree[u]<=4)eligible.push(u);
  let round=0,started=now();
  while(eligible.length){
    round++;const batch=[];
    for(const u of eligible){
      ctx.probes++;check(ctx);
      if(active[u]&&degree[u]<=4&&blocked[u]!==round){batch.push(u);blocked[u]=round;for(const v of adj[u])if(active[v])blocked[v]=round;}
    }
    if(!batch.length)throw new Error('Reduction batch made no progress.');
    for(const u of batch){active[u]=0;removed.push(u);}ctx.removed=removed.length;
    const next=[];
    const enqueue=u=>{if(active[u]&&degree[u]<=4&&queued[u]!==round){queued[u]=round;next.push(u);}};
    for(const u of batch)for(const v of adj[u]){ctx.probes++;check(ctx);if(active[v]){degree[v]--;enqueue(v);}}
    for(const u of eligible)enqueue(u);
    eligible=next;ctx.batches++;
  }
  ctx.removed=removed.length;ctx.reductionMs=now()-started;
  const core=[];for(let u=0;u<n;u++)if(active[u])core.push(u);ctx.coreVertices=core.length;
  started=now();progressState(ctx,colors,'Searching core');if(core.length)dsatur(adj,core,colors,ctx);ctx.coreMs=now()-started;
  started=now();progressState(ctx,colors,'Restoring');
  const seen=new Uint32Array(n),queue=new Uint32Array(n);let stamp=0;
  for(let i=removed.length-1;i>=0;i--){
    const u=removed[i],ring=[];let used=0;
    for(const v of adj[u])if(colors[v]>=0){ring.push(v);used|=1<<colors[v];}
    if(ring.length>4)throw new Error('Restoration degree invariant failed.');
    let free=15&~used;
    if(!free){
      let found=false;
      for(let a=0;a<ring.length&&!found;a++)for(let b=a+1;b<ring.length&&!found;b++){
        const start=ring[a],target=ring[b],ca=colors[start],cb=colors[target];stamp++;
        let head=0,tail=1;queue[0]=start;seen[start]=stamp;
        while(head<tail){
          const x=queue[head++];ctx.kempeVisits++;
          for(const y of adj[x]){ctx.probes++;check(ctx);if(seen[y]!==stamp&&(colors[y]===ca||colors[y]===cb)){seen[y]=stamp;queue[tail++]=y;}}
        }
        if(seen[target]!==stamp){
          // A half-finished Kempe swap can have temporary conflicts. Publish
          // only after the entire component has changed colors.
          if(live)live.safe=false;
          for(let z=0;z<tail;z++){const v=queue[z];colors[v]=colors[v]===ca?cb:ca;ctx.probes++;check(ctx);}
          if(live)live.safe=true;
          free=1<<ca;ctx.swaps++;found=true;
        }
      }
      if(!found)throw new Stop('unsupported','No supported degree-four Kempe extension was found.');
    }
    const bit=free&-free;colors[u]=bit===1?0:bit===2?1:bit===4?2:3;ctx.probes++;check(ctx);
  }
  ctx.restorationMs=now()-started;return colors;
}
function execute(graph,method,budgetMs,onProgress=null){
  const ctx=context(budgetMs),started=now();
  if(onProgress)liveRuns.set(ctx,{report:onProgress,started,nextAt:started+5000,colors:null,phase:'Starting',safe:true});
  try{
    const colors=method==='dsatur'?classic(graph.adj,ctx):reduction(graph.adj,ctx);
    check(ctx,true);
    return{method,status:'complete',solverMs:now()-started,colors,stats:ctx};
  }catch(error){
    reportProgress(ctx,now(),true);
    return{method,status:error.status||'error',solverMs:now()-started,message:error.message,colors:null,stats:ctx};
  }finally{liveRuns.delete(ctx);}
}
self.onmessage=event=>{
  const request=event.data;
  try{
    if(request.type==='generate'){
      const warm=makeMap(8,11);for(let i=0;i<2;i++){execute(warm,'dsatur',1000);execute(warm,'reduction',1000);}
      current=makeMap(request.side,request.seed,request.rows);
      const {n,side,rows,seed,edges,xy,offsets,fingerprint,generationMs}=current;
      self.postMessage({type:'generated',requestId:request.requestId,n,side,rows,seed,edges,xy,offsets,fingerprint,generationMs});
    }else if(request.type==='solve'){
      if(!current)throw new Error('Generate a map first.');
      if(!['dsatur','reduction'].includes(request.method))throw new Error('Unknown solver.');
      const budget=Math.min(100000,Math.max(100,Number(request.budgetMs)||100000));
      const onProgress=progress=>self.postMessage({type:'progress',requestId:request.requestId,method:request.method,...progress});
      self.postMessage({type:'solved',requestId:request.requestId,...execute(current,request.method,budget,onProgress)});
    }
  }catch(error){self.postMessage({type:'error',requestId:request.requestId,message:error.message});}
};
