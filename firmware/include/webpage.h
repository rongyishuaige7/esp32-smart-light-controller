#ifndef SMART_LIGHT_WEBPAGE_H
#define SMART_LIGHT_WEBPAGE_H

// Local, self-contained control page. It deliberately has no external fonts,
// icon CDN, telemetry or cross-origin requests.
const char CONTROL_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>多路智能照明控制</title>
  <style>
    :root{color-scheme:dark;--bg:#101827;--card:#192235;--line:#334155;--text:#f8fafc;--muted:#b6c2d1;--on:#22c55e;--accent:#38bdf8;--warn:#fbbf24}
    *{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
    main{max-width:720px;margin:auto;padding:20px 14px 36px}h1{font-size:1.45rem;margin:0 0 4px}.lead,.hint{color:var(--muted);font-size:.92rem}.notice{border-left:3px solid var(--warn);padding:10px 12px;background:#27231a;margin:20px 0;border-radius:5px}.panel{background:var(--card);border:1px solid var(--line);padding:16px;border-radius:12px;margin:12px 0}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.channel{min-height:156px}.row{display:flex;align-items:center;justify-content:space-between;gap:8px}.status{font-size:1.6rem;font-weight:700;margin:4px 0}.on{color:var(--on)}.unavailable{color:var(--warn)}label{display:block;margin-top:12px;color:var(--muted);font-size:.9rem}select,input[type=range]{width:100%;margin-top:5px}button{border:0;border-radius:8px;padding:9px 13px;background:#334155;color:var(--text);font:inherit;cursor:pointer}button.active{background:var(--on);color:#09210f}button:focus-visible,input:focus-visible,select:focus-visible{outline:3px solid var(--accent);outline-offset:2px}.message{min-height:1.5em;color:var(--muted)}@media(max-width:480px){.grid{grid-template-columns:1fr}}
  </style>
</head>
<body>
  <main>
    <h1>多路智能照明控制</h1>
    <p class="lead">ESP32 本地教学原型 · 仅限可信局域网与低压 LED 负载</p>
    <p class="notice">本页不表示设备在线、控制必达或家用安全能力。没有 BH1750 时，自动模式不会改变灯光输出。</p>
    <section class="panel" aria-live="polite">
      <div class="row"><strong>环境光照</strong><span id="sensor">读取中…</span></div>
      <div id="lux" class="status">—</div>
      <div class="hint">传感器状态只描述本次本地读取，不代表已校准。</div>
    </section>
    <section id="channels" class="grid" aria-label="四路低压 LED 控制"></section>
    <section class="panel">
      <div class="row"><strong>自动模式</strong><button id="auto" type="button" aria-pressed="false">关闭</button></div>
      <label for="threshold">低于此阈值时自动打开全部四路；高于阈值 + 10 lux 时关闭全部四路：<output id="thresholdValue">50 lux</output></label>
      <input id="threshold" type="range" min="5" max="300" value="50">
      <p class="hint">自动模式会同时覆盖四路灯光并清除各路倒计时；这不是逐路自动控制。</p>
    </section>
    <p id="message" class="message" role="status"></p>
  </main>
  <script>
    const COUNTDOWNS=[{value:0,label:'不设倒计时'},{value:300,label:'5 分钟后关闭'},{value:900,label:'15 分钟后关闭'},{value:1800,label:'30 分钟后关闭'},{value:3600,label:'60 分钟后关闭'}];
    const channels=document.getElementById('channels');const message=document.getElementById('message');let state=null;
    function tell(text){message.textContent=text;}
    async function request(path,body){const response=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});const result=await response.json().catch(()=>({code:'invalid_response'}));if(!response.ok)throw new Error(result.code||'request_failed');return result;}
    function formatRemaining(seconds){if(!seconds)return '未运行';return Math.floor(seconds/60)+':'+String(seconds%60).padStart(2,'0');}
    function draw(data){state=data;const unavailable=data.sensor!=='available';document.getElementById('sensor').textContent=unavailable?'不可用':'可用';document.getElementById('sensor').className=unavailable?'unavailable':'on';document.getElementById('lux').textContent=unavailable?'—':Math.round(data.lux)+' lux';document.getElementById('threshold').value=data.thresholdLux;document.getElementById('thresholdValue').textContent=data.thresholdLux+' lux';const auto=document.getElementById('auto');auto.textContent=data.autoModeEnabled?'开启':'关闭';auto.classList.toggle('active',data.autoModeEnabled);auto.setAttribute('aria-pressed',String(data.autoModeEnabled));channels.replaceChildren(...data.leds.map((led)=>{const card=document.createElement('section');card.className='panel channel';const title=document.createElement('div');title.className='row';title.innerHTML='<strong>第 '+(led.id+1)+' 路 LED</strong><span class="'+(led.enabled?'on':'')+'">'+(led.enabled?'开启':'关闭')+'</span>';const toggle=document.createElement('button');toggle.type='button';toggle.textContent=led.enabled?'关闭该路':'开启该路';toggle.className=led.enabled?'active':'';toggle.onclick=async()=>{try{await request('/api/led',{id:led.id,enabled:!led.enabled,countdownSeconds:0});tell('已请求更新第 '+(led.id+1)+' 路。');await refresh();}catch(error){tell('更新失败：'+error.message);}};const label=document.createElement('label');label.textContent='倒计时（手动设置会开启该路）';const select=document.createElement('select');COUNTDOWNS.forEach(item=>{const option=new Option(item.label,item.value,false,Number(item.value)===led.countdownSeconds);select.add(option);});select.onchange=async()=>{try{await request('/api/led',{id:led.id,enabled:true,countdownSeconds:Number(select.value)});tell('已请求设置第 '+(led.id+1)+' 路倒计时。');await refresh();}catch(error){tell('设置失败：'+error.message);}};const remaining=document.createElement('p');remaining.className='hint';remaining.textContent='剩余：'+formatRemaining(led.countdownLeftSeconds);card.append(title,toggle,label,select,remaining);return card;}));}
    async function refresh(){try{const response=await fetch('/api/status',{cache:'no-store'});if(!response.ok)throw new Error('status_'+response.status);draw(await response.json());tell('已读取本次本地状态。');}catch(error){tell('无法读取本次状态：'+error.message);}}
    document.getElementById('auto').onclick=async()=>{if(!state)return;try{await request('/api/config',{autoModeEnabled:!state.autoModeEnabled,thresholdLux:Number(document.getElementById('threshold').value)});tell('已请求更新自动模式。');await refresh();}catch(error){tell('更新失败：'+error.message);}};
    document.getElementById('threshold').onchange=async(event)=>{if(!state)return;try{await request('/api/config',{autoModeEnabled:state.autoModeEnabled,thresholdLux:Number(event.target.value)});tell('已请求更新阈值。');await refresh();}catch(error){tell('更新失败：'+error.message);}};
    refresh();setInterval(refresh,3000);
  </script>
</body>
</html>
)rawliteral";

#endif
