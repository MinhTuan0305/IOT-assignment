#ifndef TRANG_WEB_H
#define TRANG_WEB_H

// ============ TRANG WEB (HTML+CSS+JS) - TACH RIENG KHOI FILE LOGIC ============
const char TRANG_CHINH[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0b1220">
<title>Hệ sinh thái cây</title>
<style>
  :root {
    --bg-a:#0b1220; --bg-b:#111c30;
    --card:#141e33; --card-border:#223049;
    --text:#e7ecf5; --text-dim:#8ea0bd;
    --accent:#22c55e; --danger:#ef4444;
    --temp:#fb923c; --hum:#38bdf8; --soil:#c08a4e; --light:#facc15; --gas:#c084fc;
    --radius:16px;
    --safe-t:env(safe-area-inset-top,0px); --safe-b:env(safe-area-inset-bottom,0px);
  }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
  html,body { margin:0; overscroll-behavior-y:contain; }
  body {
    background:radial-gradient(circle at top,var(--bg-b),var(--bg-a));
    color:var(--text);
    font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    min-height:100vh;
    padding-top:var(--safe-t); padding-bottom:var(--safe-b);
  }
  .app { max-width:960px; margin:0 auto; padding:0 16px 28px; }
  .topbar {
    position:sticky; top:0; z-index:10;
    display:flex; align-items:center; justify-content:space-between;
    padding:14px 2px; margin-bottom:6px;
    backdrop-filter:blur(10px);
    background:linear-gradient(var(--bg-a) 65%, transparent);
  }
  .brand { font-size:17px; font-weight:700; display:flex; align-items:center; gap:8px; }
  .status-pill {
    display:flex; align-items:center; gap:6px; font-size:12px;
    padding:6px 11px; border-radius:999px;
    background:var(--card); border:1px solid var(--card-border); color:var(--text-dim);
  }
  .status-pill .dot { width:8px; height:8px; border-radius:50%; background:var(--danger); transition:background .3s; }
  .status-pill.ok .dot { background:var(--accent); box-shadow:0 0 0 3px rgba(34,197,94,0.22); }

  .content { display:flex; flex-direction:column; gap:4px; }
  h2.section-title { font-size:12px; text-transform:uppercase; letter-spacing:.06em; color:var(--text-dim); margin:20px 2px 10px; }

  .sensors { display:grid; grid-template-columns:repeat(auto-fit,minmax(135px,1fr)); gap:10px; }
  .sensor-card {
    background:var(--card); border:1px solid var(--card-border); border-radius:var(--radius);
    padding:14px; display:flex; align-items:center; gap:10px; border-left:3px solid var(--c,var(--accent));
  }
  .sensor-card.temp{ --c:var(--temp); } .sensor-card.hum{ --c:var(--hum); }
  .sensor-card.soil{ --c:var(--soil); } .sensor-card.light{ --c:var(--light); } .sensor-card.gas{ --c:var(--gas); }
  .sensor-icon { font-size:22px; line-height:1; }
  .sensor-label { font-size:11px; color:var(--text-dim); text-transform:uppercase; letter-spacing:.04em; }
  .sensor-value { font-size:19px; font-weight:700; margin-top:2px; font-variant-numeric:tabular-nums; }
  .sensor-sub { font-size:10px; color:var(--text-dim); margin-top:1px; }

  .charts { display:grid; gap:10px; }
  .chart-card { background:var(--card); border:1px solid var(--card-border); border-radius:var(--radius); padding:14px; }
  .chart-head { display:flex; justify-content:space-between; font-size:12px; color:var(--text-dim); margin-bottom:8px; }
  .chart-range { font-variant-numeric:tabular-nums; }
  canvas.bd { width:100%; height:100px; display:block; }

  .threshold-grid { display:grid; grid-template-columns:1fr 1fr; gap:8px; }
  .threshold-item { background:var(--card); border:1px solid var(--card-border); border-radius:12px; padding:10px 12px; font-size:11.5px; color:var(--text-dim); }
  .threshold-item b { display:block; font-size:16px; margin-top:3px; color:var(--text); font-variant-numeric:tabular-nums; }

  .mode-switch { display:flex; background:var(--card); border:1px solid var(--card-border); border-radius:999px; padding:4px; }
  .mode-opt {
    flex:1; border:none; background:transparent; color:var(--text-dim);
    padding:12px 10px; border-radius:999px; font-weight:600; font-size:13px; cursor:pointer;
    transition:.2s; touch-action:manipulation;
  }
  .mode-opt.active { background:var(--accent); color:#04160c; }

  .toggle-list { display:flex; flex-direction:column; gap:10px; margin-top:14px; }
  .toggle-row {
    display:flex; align-items:center; justify-content:space-between;
    background:var(--card); border:1px solid var(--card-border); border-radius:14px; padding:13px 16px;
  }
  .toggle-row span { font-size:14px; font-weight:600; display:flex; align-items:center; gap:9px; }
  .toggle {
    width:54px; height:32px; border-radius:999px; background:#334155;
    position:relative; cursor:pointer; transition:background .25s; flex-shrink:0; touch-action:manipulation;
  }
  .toggle.on { background:var(--accent); }
  .toggle .knob {
    position:absolute; top:3px; left:3px; width:26px; height:26px; border-radius:50%;
    background:#fff; transition:left .25s; box-shadow:0 1px 3px rgba(0,0,0,.3);
  }
  .toggle.on .knob { left:25px; }
  .toggle.disabled { opacity:.4; pointer-events:none; }
  .hint { font-size:11px; font-weight:400; color:var(--text-dim); }
  .note { font-size:11.5px; color:var(--text-dim); margin:12px 2px 0; line-height:1.5; }

  @media (min-width:720px) {
    .content { display:grid; grid-template-columns:1.3fr 1fr; align-items:start; gap:22px; }
  }
</style>
</head>
<body>
<div class="app">
  <header class="topbar">
    <div class="brand">🌱 <span>Hệ Sinh Thái Cây</span></div>
    <div class="status-pill" id="statusPill"><span class="dot"></span><span id="statusText">Đang tải...</span></div>
  </header>

  <div class="content">
    <div class="col-left">
      <div class="sensors">
        <div class="sensor-card temp"><div class="sensor-icon">🌡️</div>
          <div><div class="sensor-label">Nhiệt độ</div><div class="sensor-value" id="v_temp">--</div></div></div>
        <div class="sensor-card hum"><div class="sensor-icon">💧</div>
          <div><div class="sensor-label">Độ ẩm KK</div><div class="sensor-value" id="v_hum">--</div></div></div>
        <div class="sensor-card soil"><div class="sensor-icon">🪴</div>
          <div><div class="sensor-label">Đất</div><div class="sensor-value" id="v_soil">--</div><div class="sensor-sub" id="v_soil_raw"></div></div></div>
        <div class="sensor-card light"><div class="sensor-icon">☀️</div>
          <div><div class="sensor-label">Ánh sáng</div><div class="sensor-value" id="v_lux">--</div></div></div>
        <div class="sensor-card gas"><div class="sensor-icon">💨</div>
          <div><div class="sensor-label">Khí gas</div><div class="sensor-value" id="v_gas">--</div></div></div>
      </div>

      <h2 class="section-title">Biểu đồ theo thời gian</h2>
      <div class="charts">
        <div class="chart-card">
          <div class="chart-head"><span>🌡️ Nhiệt độ (°C)</span><span class="chart-range" id="minmax_temp">--</span></div>
          <canvas class="bd" id="bd_temp"></canvas>
        </div>
        <div class="chart-card">
          <div class="chart-head"><span>💧 Độ ẩm không khí (%)</span><span class="chart-range" id="minmax_hum">--</span></div>
          <canvas class="bd" id="bd_hum"></canvas>
        </div>
      </div>
    </div>

    <div class="col-right">
      <h2 class="section-title">Ngưỡng tự động đang áp dụng</h2>
      <div class="threshold-grid">
        <div class="threshold-item">Quạt bật khi nhiệt độ &gt;<b id="ng_nhiet">--</b></div>
        <div class="threshold-item">Sương bật khi độ ẩm &lt;<b id="ng_am">--</b></div>
        <div class="threshold-item">Đèn bật khi ánh sáng &lt;<b id="ng_sang">--</b></div>
        <div class="threshold-item">Còi báo khi gas &gt;<b id="ng_gas">--</b></div>
        <div class="threshold-item">Bơm tưới khi đất &lt;<b id="ng_dat">--</b></div>
      </div>

      <h2 class="section-title">Điều khiển</h2>
      <div class="mode-switch">
        <button class="mode-opt" id="modeThuCong" onclick="datCheDo(false)">Thủ công</button>
        <button class="mode-opt" id="modeTuDong" onclick="datCheDo(true)">Tự động</button>
      </div>

      <div class="toggle-list">
        <div class="toggle-row"><span>💡 Đèn</span>
          <div class="toggle" id="tg_den" onclick="bam('den')"><div class="knob"></div></div></div>
        <div class="toggle-row"><span>🌀 Quạt</span>
          <div class="toggle" id="tg_quat" onclick="bam('quat')"><div class="knob"></div></div></div>
        <div class="toggle-row"><span>💦 Sương</span>
          <div class="toggle" id="tg_suong" onclick="bam('suong')"><div class="knob"></div></div></div>
        <div class="toggle-row"><span>🚿 Bơm nước <small class="hint">tự tắt sau 5s</small></span>
          <div class="toggle" id="tg_bom" onclick="bam('bom')"><div class="knob"></div></div></div>
      </div>
      <p class="note">Khi bơm chạy, quạt và sương tạm tắt 5 giây (dùng chung nguồn 5V-1A) rồi tự bật lại.</p>
    </div>
  </div>
</div>

<script>
var lichSuNhiet = [];
var lichSuAm = [];
var SO_DIEM_TOI_DA = 60; // 60 diem x 2s ~ 2 phut gan nhat
var trangThaiHienTai = { den:false, quat:false, suong:false, bom:false };

var pill = document.getElementById('statusPill');
var statusText = document.getElementById('statusText');

function capNhat() {
  fetch('/data').then(function(r){ return r.json(); }).then(function(d) {
    document.getElementById('v_temp').innerText = d.t + ' °C';
    document.getElementById('v_hum').innerText = d.h + ' %';
    document.getElementById('v_soil').innerText = d.soilpct + '%';
    document.getElementById('v_soil_raw').innerText = 'raw: ' + d.soil;
    document.getElementById('v_lux').innerText = d.lux;
    document.getElementById('v_gas').innerText = d.gas;

    datToggle('tg_den', d.den);
    datToggle('tg_quat', d.quat);
    datToggle('tg_suong', d.suong);
    datToggle('tg_bom', d.bom);
    trangThaiHienTai.den = !!d.den;
    trangThaiHienTai.quat = !!d.quat;
    trangThaiHienTai.suong = !!d.suong;
    trangThaiHienTai.bom = !!d.bom;

    document.getElementById('modeThuCong').classList.toggle('active', !d.mode);
    document.getElementById('modeTuDong').classList.toggle('active', !!d.mode);
    ['tg_den','tg_quat','tg_suong','tg_bom'].forEach(function(id){
      document.getElementById(id).classList.toggle('disabled', d.mode == 1);
    });

    document.getElementById('ng_nhiet').innerText = d.ngt + ' °C';
    document.getElementById('ng_am').innerText = d.nga + ' %';
    document.getElementById('ng_sang').innerText = d.ngs;
    document.getElementById('ng_gas').innerText = d.ngg;
    document.getElementById('ng_dat').innerText = d.ngd + ' %';

    if (d.tuoi < 10) {
      pill.classList.add('ok');
      statusText.innerText = 'Đã kết nối UNO';
    } else {
      pill.classList.remove('ok');
      statusText.innerText = 'Mất dữ liệu UNO (' + d.tuoi + 's)';
    }

    themDiem(lichSuNhiet, d.t);
    themDiem(lichSuAm, d.h);
    veBieuDo('bd_temp', lichSuNhiet, '#fb923c', 'minmax_temp', '°C');
    veBieuDo('bd_hum', lichSuAm, '#38bdf8', 'minmax_hum', '%');
  }).catch(function() {
    pill.classList.remove('ok');
    statusText.innerText = 'Mất kết nối ESP32';
  });
}

function themDiem(mang, giaTri) {
  mang.push(giaTri);
  if (mang.length > SO_DIEM_TOI_DA) mang.shift();
}

function datToggle(id, bat) {
  document.getElementById(id).classList.toggle('on', !!bat);
}

function bam(loai) {
  // Gui lenh "dat dung trang thai" (chu HOA = bat, chu thuong = tat) dua theo
  // trang thai hien dang hien thi - dang bat thi gui lenh tat, dang tat thi gui lenh bat.
  var maChuHoa = { den:'D', quat:'Q', suong:'S', bom:'B' }[loai];
  var dangBat = trangThaiHienTai[loai];
  var maLenh = dangBat ? maChuHoa.toLowerCase() : maChuHoa;
  fetch('/cmd?c=' + maLenh).then(capNhat);
}

function datCheDo(tuDong) {
  fetch('/cmd?c=' + (tuDong ? 'a' : 'm')).then(capNhat);
}

function veBieuDo(idCanvas, data, mau, idMinMax, donVi) {
  var canvas = document.getElementById(idCanvas);
  var dpr = window.devicePixelRatio || 1;
  var cssW = canvas.clientWidth || 300, cssH = canvas.clientHeight || 100;
  var w = Math.round(cssW * dpr), h = Math.round(cssH * dpr);
  if (canvas.width !== w) { canvas.width = w; canvas.height = h; }
  var ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, cssW, cssH);
  if (data.length < 2) return;

  var minV = Math.min.apply(null, data), maxV = Math.max.apply(null, data);
  if (minV === maxV) { minV -= 1; maxV += 1; }
  var bienDo = maxV - minV;
  var padY = cssH * 0.12;
  var caoVe = cssH - padY * 2;

  function toaY(v) { return padY + caoVe - ((v - minV) / bienDo) * caoVe; }

  ctx.strokeStyle = 'rgba(148,163,184,0.14)';
  ctx.lineWidth = 1;
  for (var i = 0; i <= 3; i++) {
    var y = padY + (caoVe / 3) * i;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(cssW, y); ctx.stroke();
  }

  var diem = data.map(function (v, i) {
    return { x: (i / (data.length - 1)) * cssW, y: toaY(v) };
  });

  var gradient = ctx.createLinearGradient(0, 0, 0, cssH);
  gradient.addColorStop(0, mau + '40');
  gradient.addColorStop(1, mau + '00');
  ctx.beginPath();
  ctx.moveTo(diem[0].x, cssH);
  diem.forEach(function (p) { ctx.lineTo(p.x, p.y); });
  ctx.lineTo(diem[diem.length - 1].x, cssH);
  ctx.closePath();
  ctx.fillStyle = gradient;
  ctx.fill();

  ctx.strokeStyle = mau;
  ctx.lineWidth = 2.5;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  diem.forEach(function (p, i) { if (i === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
  ctx.stroke();

  var cuoi = diem[diem.length - 1];
  ctx.fillStyle = mau;
  ctx.beginPath(); ctx.arc(cuoi.x, cuoi.y, 3.5, 0, Math.PI * 2); ctx.fill();

  document.getElementById(idMinMax).innerText = minV.toFixed(1) + ' - ' + maxV.toFixed(1) + donVi;
}

setInterval(capNhat, 2000);
window.addEventListener('resize', function () {
  veBieuDo('bd_temp', lichSuNhiet, '#fb923c', 'minmax_temp', '°C');
  veBieuDo('bd_hum', lichSuAm, '#38bdf8', 'minmax_hum', '%');
});
capNhat();
</script>
</body>
</html>
)HTMLPAGE";

#endif
