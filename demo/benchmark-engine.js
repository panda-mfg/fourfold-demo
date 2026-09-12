/* Build concatenates map-generator.js and wasm-adapter.js before this entry.
 * Rust/WASM handles both measured solvers. Compilation, graph import and warm-up
 * happen before timing; progress-copying overhead is included in solver time.
 */
'use strict';
const WASM_BASE64='__FOURFOLD_WASM_BASE64__';
let enginePromise=null,current=null,queue=Promise.resolve();
function engine(){
  if(!enginePromise)enginePromise=createWasmEngine(Uint8Array.from(atob(WASM_BASE64),c=>c.charCodeAt(0)));
  return enginePromise;
}
async function handle(request){
  try{
    const rust=await engine();
    if(request.type==='generate'){
      const warm=makeMap(8,11);rust.prepare(warm);
      for(let i=0;i<2;i++){rust.execute('dsatur',1000);rust.execute('reduction',1000);}
      current=makeMap(request.side,request.seed,request.rows);rust.prepare(current);
      const {n,side,rows,seed,edges,xy,offsets,fingerprint,generationMs}=current;
      self.postMessage({type:'generated',requestId:request.requestId,backend:'rust-wasm',n,side,rows,seed,edges,xy,offsets,fingerprint,generationMs});
    }else if(request.type==='solve'){
      if(!current)throw new Error('Generate a map first.');
      const budget=Math.min(100000,Math.max(100,Number(request.budgetMs)||100000));
      const report=progress=>self.postMessage({type:'progress',requestId:request.requestId,method:request.method,backend:'rust-wasm',...progress});
      self.postMessage({type:'solved',requestId:request.requestId,...rust.execute(request.method,budget,report)});
    }else throw new Error('Unknown worker request.');
  }catch(error){self.postMessage({type:'error',requestId:request.requestId,message:error.message});}
}
self.onmessage=event=>{queue=queue.then(()=>handle(event.data));};
