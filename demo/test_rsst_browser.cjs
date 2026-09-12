const {chromium}=require(process.env.FOURFOLD_PLAYWRIGHT || 'playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),os=require('node:os');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.FOURFOLD_CHROME,args:process.env.FOURFOLD_NO_SANDBOX==='1'?['--no-sandbox']:[]});
 try{
  const page=await browser.newPage({viewport:{width:1120,height:1800}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));await page.goto(process.env.FOURFOLD_URL || 'http://127.0.0.1:8000/');
  const f=page.frames().find(x=>x.parentFrame()),panel=f.locator('[data-b-method="baseline"]');
  const idle=(timeout=30000)=>f.waitForFunction(()=>!document.querySelector('[data-b-run]').disabled,null,{timeout});
  async function latest(){await f.locator('[data-b-export]').click();const data=JSON.parse(await f.locator('[data-b-json]').inputValue());await f.locator('[data-b-export]').click();return data.records.at(-1);}
  await idle();assert.equal(await f.locator('[data-b-baseline]').inputValue(),'rsst');
  await f.locator('[data-b-size]').selectOption('64x32');await f.locator('[data-b-repeats]').selectOption('1');await f.locator('[data-b-threads]').selectOption('1');
  await f.locator('[data-b-run]').click();await idle(110000);const single=await latest();assert.equal(single.schemaVersion,4);assert.equal(single.baseline,'rsst');assert.equal(single.trials.baseline[0].method,'rsst');
  await f.locator('[data-b-threads]').selectOption('4');await f.locator('[data-b-repeats]').selectOption('3');await f.locator('[data-b-run]').click();await idle(110000);const parallel=await latest();
  for(const record of [single,parallel])for(const method of ['baseline','reduction'])for(const trial of record.trials[method]){
   assert.equal(trial.status,'complete',trial.message);assert.equal(trial.validation.ok,true);assert.equal(trial.threads,record.threads);assert.equal(trial.attempts.length,record.threads);
  }
  assert.ok(parallel.trials.baseline.some(t=>t.rsst.configurations>0));
  await f.locator('#fourfold-exhibit').screenshot({path:path.join(os.tmpdir(),'fourfold-rsst-desktop.png')});
  await page.setViewportSize({width:390,height:2800});await page.emulateMedia({colorScheme:'dark'});
  assert.ok(await f.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await f.locator('#fourfold-exhibit').screenshot({path:path.join(os.tmpdir(),'fourfold-rsst-mobile.png')});
  await f.locator('[data-b-baseline]').selectOption('dsatur');await f.locator('[data-b-size]').selectOption('8');await f.locator('[data-b-repeats]').selectOption('1');await f.locator('[data-b-run]').click();await idle();
  const diagnostic=await latest();assert.equal(diagnostic.baseline,'dsatur');assert.equal(diagnostic.trials.baseline[0].method,'dsatur');assert.equal(diagnostic.trials.baseline[0].validation.ok,true);assert.match(await panel.locator('h3').textContent(),/DSATUR/);
  await f.locator('[data-b-baseline]').selectOption('rsst');await f.locator('[data-b-size]').selectOption('128');await f.locator('[data-b-threads]').selectOption('1');await f.locator('[data-b-run]').click();
  await f.waitForFunction(()=>document.querySelector('[data-b-method="baseline"] [data-b-status]').textContent==='Running');
  assert.equal(await f.locator('[data-b-baseline]').isDisabled(),true);await f.locator('[data-b-stop]').click();assert.equal(await panel.locator('[data-b-status]').textContent(),'Cancelled');
  await f.locator('[data-b-size]').selectOption('8');await f.locator('[data-b-threads]').selectOption('2');await f.locator('[data-b-run]').click();await idle();const restarted=await latest();assert.equal(restarted.trials.baseline[0].validation.ok,true);
  await f.locator('[data-b-mode="play"]').click();assert.equal(await f.locator('[data-b-play-view]').isVisible(),true);assert.deepEqual(errors,[]);
  const result={single,parallel,diagnostic,restarted,checks:['RSST default','actual WASM Web Workers','1 and 4 workers','edge validation','schema 4 with RSST counters','DSATUR diagnostic selector','stop/restart','responsive light/dark','paint mode'],errors};
  fs.writeFileSync(path.join(__dirname,'../verification/results/rsst/browser-tests.json'),JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify({singleMs:single.trials.baseline[0].solverMs,parallelMs:parallel.trials.baseline.map(t=>t.solverMs),checks:result.checks,errors}));
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
