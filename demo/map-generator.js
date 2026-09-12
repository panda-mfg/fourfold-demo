/* Shared deterministic Voronoi map generator for the browser and CLI fixtures.
 * Map construction is excluded from measured solver time.
 */
'use strict';
const now = () => performance.now();
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

if(typeof module !== 'undefined')module.exports={makeMap,rng};
