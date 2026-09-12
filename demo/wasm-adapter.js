'use strict';
// Browser and Node share this adapter. The solver implementation lives in Rust.
async function createWasmEngine(bytes,clock=()=>performance.now()){
  let exports,reporter=null,vertexCount=0;
  const keys=['probes','decisions','backtracks','batches','removed','swaps','kempeVisits','coreVertices','reductionMs','coreMs','restorationMs','progressUpdates'];
  const phases=['Starting','Searching','Reducing','Searching core','Restoring'];
  const stats=pointer=>{
    const values=new Float64Array(exports.memory.buffer,pointer,13);
    return{stats:Object.fromEntries(keys.map((key,i)=>[key,values[i]])),solverMs:values[12]};
  };
  const loaded=await WebAssembly.instantiate(bytes,{host:{
    time_now:clock,
    progress:(pointer,length,elapsed,phase,statsPointer)=>{
      if(reporter)reporter({colors:new Int8Array(exports.memory.buffer,pointer,length).slice(),solverMs:elapsed,phase:phases[phase],stats:stats(statsPointer).stats});
    }
  }});
  exports=loaded.instance.exports;
  if(exports.abi_version()!==1)throw new Error('Unsupported Rust/WASM engine version.');
  return{
    backend:'rust-wasm',
    prepare(graph){
      vertexCount=0;
      const n=graph.n??graph.adj.length;
      const edges=graph.edges??Uint32Array.from(graph.adj.flatMap((neighbors,u)=>neighbors.filter(v=>u<v).flatMap(v=>[u,v])));
      if(!Number.isInteger(n)||n<1||n>16384||edges.length%2||edges.length>n*16)throw new Error('Invalid graph size.');
      for(const v of edges)if(!Number.isInteger(v)||v<0||v>=n)throw new Error('Invalid edge endpoint.');
      const pointer=exports.allocate_graph(n,edges.length);
      if(!pointer)throw new Error('Rust graph allocation failed.');
      new Uint32Array(exports.memory.buffer,pointer,edges.length).set(edges);
      if(exports.prepare_graph()!==0)throw new Error('Invalid graph: loops or duplicate edges.');
      vertexCount=n;
    },
    execute(method,budgetMs=100000,onProgress=null){
      if(!['dsatur','reduction'].includes(method))throw new Error('Unknown solver.');
      if(!vertexCount)throw new Error('Prepare a graph first.');
      if(!Number.isFinite(budgetMs)||budgetMs<=0)throw new Error('Invalid time budget.');
      reporter=onProgress;
      try{
        const code=exports.solve(method==='dsatur'?0:1,Math.min(100000,budgetMs),onProgress?1:0);
        const status=['complete','timeout','unsupported','error'][code]??'error';
        const result={method,backend:'rust-wasm',status,...stats(exports.stats_ptr()),colors:null};
        if(status==='complete'){
          if(exports.colors_len()!==vertexCount)throw new Error('Rust returned the wrong color count.');
          result.colors=new Int8Array(exports.memory.buffer,exports.colors_ptr(),vertexCount).slice();
        }else result.message=code===1?'Time limit reached.':code===2?'No supported four-coloring was found.':'Rust solver rejected the request.';
        return result;
      }finally{reporter=null;}
    }
  };
}
if(typeof module!=='undefined')module.exports={createWasmEngine};
