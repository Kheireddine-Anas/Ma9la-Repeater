#pragma once
#include <pgmspace.h>

// HTML is in a .h file so the Arduino preprocessor does not touch it.
// Raw string literals in .ino files confuse the Arduino IDE preprocessor;
// .h files are passed straight to the C++ compiler and work correctly.

static const char INDEX_HTML[] PROGMEM = R"EOF(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ma9la-NET</title>
<style>
:root{--bg:#060a14;--card:#0c1425;--bd:#1a2a4a;--cyan:#00c8ff;--pur:#a855f7;--grn:#22c55e;--red:#ef4444;--ora:#f59e0b;--txt:#e2e8f0;--mut:#64748b}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--txt);font-family:system-ui,-apple-system,sans-serif;min-height:100vh}
.hdr{text-align:center;padding:28px 20px 14px}
.logo{font-size:2rem;font-weight:800;letter-spacing:3px;background:linear-gradient(135deg,var(--cyan),var(--pur));-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
.sub{color:var(--mut);font-size:.7rem;letter-spacing:3px;text-transform:uppercase;margin-top:4px}
.wrap{max-width:480px;margin:0 auto;padding:0 14px 48px}
.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:18px;margin-bottom:12px}
.ctitle{font-size:.65rem;font-weight:700;letter-spacing:2px;text-transform:uppercase;color:var(--mut);margin-bottom:14px;display:flex;align-items:center;justify-content:space-between}
.srow{display:flex;align-items:center;gap:10px}
.dot{width:9px;height:9px;border-radius:50%;flex-shrink:0}
.dg{background:var(--grn);box-shadow:0 0 8px var(--grn);animation:pg 2s infinite}
.dr{background:var(--red);box-shadow:0 0 8px var(--red)}
.do{background:var(--ora);box-shadow:0 0 8px var(--ora)}
@keyframes pg{0%,100%{opacity:1}50%{opacity:.35}}
.slbl{font-size:.95rem;font-weight:600}
.sgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:12px}
.si{background:#0a1222;border-radius:8px;padding:10px}
.silbl{font-size:.6rem;text-transform:uppercase;letter-spacing:1px;color:var(--mut);margin-bottom:3px}
.sival{font-size:.85rem;font-weight:600;color:var(--cyan);word-break:break-all;display:flex;align-items:center;gap:6px}
.bars{display:flex;align-items:flex-end;gap:2px;height:14px}
.b{width:4px;border-radius:1px;background:var(--bd)}
.b:nth-child(1){height:25%}.b:nth-child(2){height:50%}.b:nth-child(3){height:75%}.b:nth-child(4){height:100%}
.b.on{background:var(--grn)}.b.yw{background:var(--ora)}.b.rv{background:var(--red)}
input[type=text],input[type=password]{width:100%;padding:10px 12px;background:#0a1222;border:1px solid var(--bd);border-radius:8px;color:var(--txt);font-size:.9rem;outline:none;transition:border-color .2s}
input:focus{border-color:var(--cyan)}
input[type=checkbox]{width:auto;margin-right:6px}
label{display:block;font-size:.65rem;text-transform:uppercase;letter-spacing:1px;color:var(--mut);margin-bottom:5px;margin-top:12px}
label:first-child{margin-top:0}
.chk-row{display:flex;align-items:center;margin-top:12px}
.chk-row label{margin:0;text-transform:none;font-size:.85rem;color:var(--txt);letter-spacing:0}
.btn{width:100%;padding:11px;border:none;border-radius:8px;font-size:.85rem;font-weight:700;cursor:pointer;letter-spacing:1px;transition:opacity .2s;margin-top:10px}
.btn:hover{opacity:.82}
.bp{background:linear-gradient(135deg,var(--cyan),var(--pur));color:#fff}
.bd2{background:var(--red);color:#fff}
.bo{background:transparent;border:1px solid var(--cyan);color:var(--cyan);margin-top:0}
.bwarn{background:transparent;border:1px solid var(--ora);color:var(--ora)}
.spin{display:inline-block;width:12px;height:12px;border:2px solid transparent;border-top-color:var(--cyan);border-radius:50%;animation:sp .7s linear infinite;vertical-align:middle;margin-right:5px}
@keyframes sp{to{transform:rotate(360deg)}}
#slist{margin-top:10px;display:flex;flex-direction:column;gap:5px}
.ni{display:flex;align-items:center;justify-content:space-between;padding:9px 11px;background:#0a1222;border:1px solid var(--bd);border-radius:8px;cursor:pointer;transition:border-color .15s}
.ni:hover,.ni.sel{border-color:var(--cyan);background:rgba(0,200,255,.05)}
.nname{font-size:.85rem;font-weight:500;display:flex;align-items:center;gap:5px;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.lk{font-size:.7rem;color:var(--mut);flex-shrink:0}
.nr{display:flex;align-items:center;gap:7px;flex-shrink:0;margin-left:8px}
.rssi{font-size:.65rem;color:var(--mut)}
.saved-item{display:flex;align-items:center;justify-content:space-between;padding:9px 11px;background:#0a1222;border:1px solid var(--bd);border-radius:8px;margin-bottom:5px}
.saved-item.act{border-color:var(--grn)}
.sname{font-size:.85rem;font-weight:500;display:flex;align-items:center;gap:7px;min-width:0;overflow:hidden}
.badge{font-size:.55rem;background:var(--grn);color:#000;padding:1px 5px;border-radius:3px;font-weight:800;flex-shrink:0}
.sact{display:flex;gap:5px;flex-shrink:0;margin-left:8px}
.bsm{padding:5px 9px;font-size:.7rem;font-weight:700;border-radius:6px;cursor:pointer;border:1px solid;letter-spacing:.5px;transition:opacity .2s}
.bsm:hover{opacity:.75}
.buse{background:transparent;border-color:var(--cyan);color:var(--cyan)}
.bdel{background:transparent;border-color:var(--red);color:var(--red)}
.client-item{display:flex;align-items:center;justify-content:space-between;padding:8px 11px;background:#0a1222;border:1px solid var(--bd);border-radius:8px;margin-bottom:5px;font-size:.8rem}
.mac{font-family:monospace;color:var(--cyan)}
.empty{text-align:center;color:var(--mut);font-size:.8rem;padding:8px 0}
.clbl{font-size:.65rem;color:var(--mut);margin-bottom:8px;text-align:right}
.refresh-btn{background:none;border:none;color:var(--mut);cursor:pointer;font-size:.75rem;padding:0}
.refresh-btn:hover{color:var(--cyan)}
.ftr{text-align:center;padding:16px;color:var(--mut);font-size:.7rem}
.ftr a{color:var(--pur);text-decoration:none}
.divider{border:none;border-top:1px solid var(--bd);margin:14px 0}
</style>
</head>
<body>
<div class="hdr">
  <div class="logo">Ma9la-NET</div>
  <div class="sub">Wi-Fi Repeater Control Panel</div>
</div>
<div class="wrap">

  <div class="card" id="sc">
    <div class="ctitle">Connection Status</div>
    <div id="scont"><div class="srow"><div class="dot do"></div><span class="slbl" style="color:var(--ora)">Loading&hellip;</span></div></div>
  </div>

  <div class="card">
    <div class="ctitle">
      Connected Clients
      <button class="refresh-btn" onclick="loadClients()">&#x27F3; refresh</button>
    </div>
    <div id="clients-list"><div class="empty">Loading&hellip;</div></div>
  </div>

  <div class="card">
    <div class="ctitle">Scan for Networks</div>
    <button class="btn bo" id="scanbtn" onclick="doScan()">&#x27F3; &nbsp;Scan Networks</button>
    <div id="slist"></div>
  </div>

  <div class="card">
    <div class="ctitle">Credentials</div>
    <label>Network Name (SSID)</label>
    <input type="text" id="si" placeholder="Type or select from scan">
    <label>Password</label>
    <input type="password" id="pi" placeholder="Leave blank for open networks">
    <button class="btn bp" onclick="doSave()">Save &amp; Connect</button>
  </div>

  <div class="card">
    <div class="ctitle">Saved Networks</div>
    <div id="cnt" class="clbl"></div>
    <div id="svlist"><div class="empty">Loading&hellip;</div></div>
    <button class="btn bd2" style="margin-top:12px" onclick="if(confirm('Forget ALL saved networks and reboot?'))doClear()">Forget All &amp; Reboot</button>
  </div>

  <div class="card">
    <div class="ctitle">Hotspot Settings</div>
    <label>Hotspot Name (SSID)</label>
    <input type="text" id="apSsid" placeholder="Ma9la-NET">
    <label>Hotspot Password <span style="color:var(--mut);font-size:.6rem">(min 8 chars, blank = keep current)</span></label>
    <input type="password" id="apPass" placeholder="Leave blank to keep current">
    <hr class="divider">
    <label style="color:var(--cyan);font-size:.65rem;letter-spacing:2px">Portal Authentication</label>
    <div class="chk-row">
      <input type="checkbox" id="authOn">
      <label for="authOn">Require password to access portal</label>
    </div>
    <label style="margin-top:10px">Username</label>
    <input type="text" id="authUser" placeholder="admin">
    <label>Portal Password</label>
    <input type="password" id="authPass" placeholder="Leave blank to keep current">
    <button class="btn bwarn" onclick="doSaveAp()">Save &amp; Reboot Hotspot</button>
  </div>

  <div class="card">
    <div class="ctitle">System</div>
    <button class="btn bwarn" onclick="if(confirm('Reboot the ESP32 now?'))doReboot()">&#x21BA; &nbsp;Reboot ESP32</button>
  </div>

</div>
<div class="ftr">Ma9la-Repeater v2.1 by <a href="https://github.com/Kheireddine-Anas" target="_blank">Anas Kheireddine</a> &middot; 1337 School</div>

<script>
function mkBars(rssi){
  var n=rssi>=-55?4:rssi>=-65?3:rssi>=-75?2:1;
  var c=n>=3?'on':n===2?'yw':'rv';
  var h='<div class="bars">';
  for(var i=1;i<=4;i++) h+='<div class="b'+(i<=n?' '+c:'')+'"></div>';
  return h+'</div>';
}
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');}

// ── Shared renderers ─────────────────────────────────────────────────────────
function renderStatus(d){
  var el=document.getElementById('scont');
  if(d.connected){
    el.innerHTML='<div class="srow"><div class="dot dg"></div><span class="slbl" style="color:var(--grn)">Sharing Internet &#x2713;</span></div>'
      +'<div class="sgrid">'
      +'<div class="si"><div class="silbl">Network</div><div class="sival">'+esc(d.ssid)+'</div></div>'
      +'<div class="si"><div class="silbl">Signal</div><div class="sival">'+mkBars(d.rssi)+'<span style="font-size:.65rem;color:var(--mut)">'+d.rssi+' dBm</span></div></div>'
      +'<div class="si"><div class="silbl">ESP32 IP</div><div class="sival">'+esc(d.ip)+'</div></div>'
      +'<div class="si"><div class="silbl">DNS</div><div class="sival">8.8.8.8</div></div>'
      +'<div class="si"><div class="silbl">Uptime</div><div class="sival" style="font-size:.75rem">'+esc(d.uptime)+'</div></div>'
      +'<div class="si"><div class="silbl">Reboots</div><div class="sival">'+d.boots+'</div></div>'
      +'</div>';
  } else if(d.ssid!==''){
    el.innerHTML='<div class="srow"><div class="dot do"></div><span class="slbl" style="color:var(--ora)">Connecting to '+esc(d.ssid)+'&hellip;</span></div>'
      +'<div class="sgrid" style="margin-top:12px"><div class="si"><div class="silbl">Uptime</div><div class="sival" style="font-size:.75rem">'+esc(d.uptime)+'</div></div>'
      +'<div class="si"><div class="silbl">Reboots</div><div class="sival">'+d.boots+'</div></div></div>';
  } else {
    el.innerHTML='<div class="srow"><div class="dot dr"></div><span class="slbl" style="color:var(--red)">No network saved</span></div>'
      +'<div class="sgrid" style="margin-top:12px"><div class="si"><div class="silbl">Uptime</div><div class="sival" style="font-size:.75rem">'+esc(d.uptime)+'</div></div>'
      +'<div class="si"><div class="silbl">Reboots</div><div class="sival">'+d.boots+'</div></div></div>';
  }
}

function renderSaved(nets){
  document.getElementById('cnt').textContent=nets.length+' / 10 saved';
  var lel=document.getElementById('svlist');
  if(!nets.length){lel.innerHTML='<div class="empty">No networks saved yet</div>';return;}
  lel.innerHTML=nets.map(function(n){
    return '<div class="saved-item'+(n.active?' act':'')+'"><div class="sname"><span style="overflow:hidden;text-overflow:ellipsis;white-space:nowrap">'+esc(n.ssid)+'</span>'
      +(n.active?'<span class="badge">ACTIVE</span>':'')+'</div><div class="sact">'
      +(!n.active?'<button class="bsm buse" onclick="doConnect('+n.index+')">Use</button>':'')
      +'<button class="bsm bdel" onclick="doDel('+n.index+')">&times;</button></div></div>';
  }).join('');
}

// ── Startup: one /init request instead of three separate calls ───────────────
async function loadInit(){
  try{
    var r=await fetch('/init');
    var d=await r.json();
    renderStatus(d);
    renderSaved(d.networks||[]);
    document.getElementById('apSsid').value=d.apSsid||'';
    document.getElementById('authOn').checked=d.authOn||false;
    document.getElementById('authUser').value=d.authUser||'';
  }catch(e){}
}

// ── 5-second polling (status only) ───────────────────────────────────────────
async function loadStatus(){
  try{var r=await fetch('/status');renderStatus(await r.json());}catch(e){}
}

async function loadClients(){
  try{
    var r=await fetch('/clients');
    var cl=await r.json();
    var el=document.getElementById('clients-list');
    if(!cl.length){el.innerHTML='<div class="empty">No devices connected to hotspot</div>';return;}
    el.innerHTML=cl.map(function(c){
      return '<div class="client-item"><span class="mac">'+esc(c.mac)+'</span><div class="nr">'+mkBars(c.rssi)+'<span class="rssi">'+c.rssi+' dBm</span></div></div>';
    }).join('');
  }catch(e){}
}

// Available for manual refresh after delete
async function loadSaved(){
  try{var r=await fetch('/networks');renderSaved(await r.json());}catch(e){}
}

// ── Async scan: first call starts it, then polls every 700 ms until done ─────
async function doScan(){
  var btn=document.getElementById('scanbtn');
  var list=document.getElementById('slist');
  btn.disabled=true;
  btn.innerHTML='<span class="spin"></span> Scanning&hellip;';
  list.innerHTML='';
  try{
    var nets=null;
    while(nets===null){
      var r=await fetch('/scan');
      var d=await r.json();
      if(!d.scanning){
        nets=d.networks||[];
      } else {
        await new Promise(function(res){setTimeout(res,700);});
      }
    }
    if(!nets.length){
      list.innerHTML='<div class="empty">No networks found</div>';
    } else {
      list.innerHTML=nets.map(function(n){
        var safe=n.ssid.replace(/\\/g,'\\\\').replace(/'/g,"\\'");
        return '<div class="ni" onclick="selNet(\''+safe+'\')"><div class="nname">'+(n.enc?'<span class="lk">&#x1F512;</span>':'')
          +'<span style="overflow:hidden;text-overflow:ellipsis;white-space:nowrap">'+esc(n.ssid)+'</span></div>'
          +'<div class="nr"><span class="rssi">'+n.rssi+' dBm</span>'+mkBars(n.rssi)+'</div></div>';
      }).join('');
    }
  }catch(e){
    list.innerHTML='<div class="empty" style="color:var(--red)">Scan failed &mdash; try again</div>';
  }
  btn.disabled=false;
  btn.innerHTML='&#x27F3; &nbsp;Scan Again';
}

function selNet(ssid){
  document.getElementById('si').value=ssid;
  document.getElementById('pi').value='';
  document.getElementById('pi').focus();
  document.querySelectorAll('.ni').forEach(function(el){
    var sp=el.querySelector('.nname span:last-child');
    if(sp) el.classList.toggle('sel',sp.textContent===ssid);
  });
}

async function doSave(){
  var ssid=document.getElementById('si').value.trim();
  var pass=document.getElementById('pi').value;
  if(!ssid){alert('Please enter a network name.');return;}
  var r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pass)});
  if(r.status===409){alert('Network storage full (10/10). Delete one first.');return;}
  document.open();document.write(await r.text());document.close();
}

async function doConnect(idx){
  var r=await fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'index='+idx});
  document.open();document.write(await r.text());document.close();
}

async function doDel(idx){
  if(!confirm('Remove this network?'))return;
  await fetch('/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'index='+idx});
  loadSaved();
}

async function doClear(){
  var r=await fetch('/clear',{method:'POST'});
  document.open();document.write(await r.text());document.close();
}

async function doReboot(){
  var r=await fetch('/reboot',{method:'POST'});
  document.open();document.write(await r.text());document.close();
}

async function doSaveAp(){
  var ssid=document.getElementById('apSsid').value.trim();
  var pass=document.getElementById('apPass').value;
  var authOn=document.getElementById('authOn').checked;
  var user=document.getElementById('authUser').value.trim()||'admin';
  var apass=document.getElementById('authPass').value;
  if(!ssid){alert('Hotspot name cannot be empty.');return;}
  var r=await fetch('/ap-save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'apSsid='+encodeURIComponent(ssid)+'&apPass='+encodeURIComponent(pass)
      +'&authOn='+(authOn?'1':'0')+'&authUser='+encodeURIComponent(user)+'&authPass='+encodeURIComponent(apass)});
  document.open();document.write(await r.text());document.close();
}

// Startup: 2 requests instead of 4 (init bundles status + networks + apSettings)
loadInit();
loadClients();
setInterval(loadStatus,5000);
setInterval(loadClients,10000);
</script>
</body>
</html>
)EOF";
