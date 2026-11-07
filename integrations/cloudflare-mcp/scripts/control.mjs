// Only administer your own deployment. Values come from local environment, never Git.
const command=process.argv[2];
if(!['status','start','stop','probe','enable','storage-check','gmail-check'].includes(command))
  throw Error('Usage: node scripts/control.mjs status|start|stop|probe|enable|storage-check|gmail-check');
const token=process.env.AIMILIA_ADMIN_TOKEN;
const base=new URL(process.env.AIMILIA_WORKER_URL || 'https://configure-your-own-worker.invalid');
if(base.protocol!=='https:' || base.hostname.endsWith('.invalid') || base.username || base.password || base.pathname!=='/' || base.search || base.hash)
  throw Error('Set AIMILIA_WORKER_URL to the HTTPS origin of your own Worker.');
if(!token || token.length<32)throw Error('Set AIMILIA_ADMIN_TOKEN privately in this shell.');
const r=await fetch(new URL(`/api/${command}`,base),{
  method:command==='status'?'GET':'POST',redirect:'error',signal:AbortSignal.timeout(20000),
  headers:{Authorization:`Bearer ${token}`}
});
if(!r.ok)throw Error(`Request refused: HTTP ${r.status}`);
console.log(JSON.stringify(await r.json(),null,2));
