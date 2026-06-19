class SensorMonitor {
  constructor() {
    this.updateInterval = null;
    this.countdownInterval = null;
    this.uptimeInterval = null;
    this.currentInterval = 5000;
    this.countdown = 5;
    this.isPaused = false;
    this.updateCount = 0;
    this.startTime = Date.now();

    this.tempHumChart = null;
    this.gasChart = null;
    this.dustChart = null;

    this.history = {
      timestamps: [], temperature: [], humidity: [],
      pressure: [], co2: [], co: [], lpg: [], nh3: [],
      pm1_0: [], pm2_5: [], pm10: []
    };
    this.maxHistoryPoints = 20;

    this.initialize();
  }

  initialize() {
    const start = () => {
      this.setupCharts();
      this.setupEventListeners();
      this.startPolling();
      this.startUptimeCounter();
    };
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', start);
    } else {
      start();
    }
  }

  setupCharts() {
    const th = document.getElementById('tempHumChart');
    const gas = document.getElementById('gasChart');
    const dust = document.getElementById('dustChart');
    if (th) this.tempHumChart = this.makeLineChart(th, [
      { key: 'temperature', color: '#dc2626', label: 'Температура' },
      { key: 'humidity', color: '#2563eb', label: 'Влажность' },
      { key: 'pressure', color: '#0284c7', label: 'Давление' }
    ]);
    if (gas) this.gasChart = this.makeLineChart(gas, [
      { key: 'co2', color: '#16a34a', label: 'CO₂' },
      { key: 'co', color: '#d97706', label: 'CO' },
      { key: 'lpg', color: '#7c3aed', label: 'LPG' },
      { key: 'nh3', color: '#0891b2', label: 'NH₃' }
    ]);
    if (dust) this.dustChart = this.makeLineChart(dust, [
      { key: 'pm1_0', color: '#2563eb', label: 'PM1.0' },
      { key: 'pm2_5', color: '#d97706', label: 'PM2.5' },
      { key: 'pm10', color: '#dc2626', label: 'PM10' }
    ]);
  }

  makeLineChart(canvas, series) {
    const ctx = canvas.getContext('2d');
    return {
      ctx, width: canvas.width, height: canvas.height, series,
      draw(history) {
        const c = this.ctx;
        c.clearRect(0, 0, this.width, this.height);
        if (!history || history.timestamps.length < 2) {
          c.font = '14px sans-serif';
          c.fillStyle = '#9aa5b1';
          c.textAlign = 'center';
          c.fillText('Накопление данных…', this.width / 2, this.height / 2);
          c.textAlign = 'left';
          return;
        }
        c.strokeStyle = '#eef0f3';
        c.lineWidth = 1;
        for (let i = 0; i <= 5; i++) {
          const x = i * this.width / 5, y = i * this.height / 5;
          c.beginPath(); c.moveTo(x, 0); c.lineTo(x, this.height); c.stroke();
          c.beginPath(); c.moveTo(0, y); c.lineTo(this.width, y); c.stroke();
        }
        this.series.forEach((s, i) => {
          const values = history[s.key];
          if (!values || values.length < 2) return;
          const max = Math.max(...values), min = Math.min(...values), range = max - min || 1;
          c.strokeStyle = s.color;
          c.lineWidth = 2;
          c.beginPath();
          values.forEach((v, idx) => {
            const x = idx * this.width / (values.length - 1);
            const y = this.height - ((v - min) / range * this.height * 0.8) - this.height * 0.1;
            idx === 0 ? c.moveTo(x, y) : c.lineTo(x, y);
          });
          c.stroke();
          c.fillStyle = s.color;
          c.font = '12px sans-serif';
          c.fillText(s.label, 10, 18 + i * 16);
        });
      }
    };
  }

  addToHistory(data, timestamp) {
    const h = this.history;
    h.timestamps.push(timestamp);
    h.temperature.push(data.temperature || 0);
    h.humidity.push(data.humidity || 0);
    h.pressure.push(data.pressure || 0);
    h.co2.push(data.CO2 || 0);
    h.co.push(data.CO || 0);
    h.lpg.push(data.LPG || 0);
    h.nh3.push(data.NH3 || 0);
    h.pm1_0.push(data.pm1_0 ?? 0);
    h.pm2_5.push(data.pm2_5 ?? 0);
    h.pm10.push(data.pm10 ?? 0);
    if (h.timestamps.length > this.maxHistoryPoints) {
      Object.values(h).forEach(arr => arr.shift());
    }
    this.updateCharts();
  }

  updateCharts() {
    if (this.tempHumChart) this.tempHumChart.draw(this.history);
    if (this.gasChart) this.gasChart.draw(this.history);
    if (this.dustChart) this.dustChart.draw(this.history);
  }

  startPolling() {
    this.fetchData();
    if (this.updateInterval) clearInterval(this.updateInterval);
    this.updateInterval = setInterval(() => {
      if (!this.isPaused) this.fetchData();
    }, this.currentInterval);
    this.startCountdown();
  }

  updatePollingInterval(seconds) {
    this.currentInterval = seconds * 1000;
    this.countdown = seconds;
    if (this.updateInterval) clearInterval(this.updateInterval);
    this.updateInterval = setInterval(() => {
      if (!this.isPaused) this.fetchData();
    }, this.currentInterval);
    this.setText('nextUpdate', `${this.countdown} с`);
    this.addLogEntry(`Интервал обновления: ${seconds} с`);
  }

  startCountdown() {
    if (this.countdownInterval) clearInterval(this.countdownInterval);
    this.countdownInterval = setInterval(() => {
      if (this.isPaused) return;
      this.countdown--;
      if (this.countdown <= 0) this.countdown = Math.floor(this.currentInterval / 1000);
      this.setText('nextUpdate', `${this.countdown} с`);
    }, 1000);
  }

  startUptimeCounter() {
    if (this.uptimeInterval) clearInterval(this.uptimeInterval);
    this.uptimeInterval = setInterval(() => {
      const s = Math.floor((Date.now() - this.startTime) / 1000);
      const hh = String(Math.floor(s / 3600)).padStart(2, '0');
      const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
      const ss = String(s % 60).padStart(2, '0');
      this.setText('uptime', `${hh}:${mm}:${ss}`);
    }, 1000);
  }

  async fetchData() {
    try {
      const response = await fetch('/get', { headers: { 'Cache-Control': 'no-cache' } });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      const data = await response.json();
      this.updateDisplay(data);
      this.updateCount++;
      this.addToHistory(data, new Date().toLocaleTimeString());
      this.setConnectionStatus(true, 'Подключено');
      this.setText('totalUpdates', this.updateCount);
    } catch (error) {
      this.setConnectionStatus(false, 'Нет связи');
    }
  }

  updateDisplay(data) {
    this.updateValue('temperature', data.temperature);
    this.updateValue('humidity', data.humidity);
    this.updateValue('pressure', data.pressure);
    this.updateValue('co2', data.CO2);
    this.updateValue('co', data.CO);
    this.updateValue('lpg', data.LPG);
    this.updateValue('nh3', data.NH3);
    this.updateDust(data);
    this.setText('lastUpdate', new Date().toLocaleTimeString());
    this.updateSensorStatus(data);
  }

  updateValue(id, value) {
    const el = document.getElementById(id);
    if (!el) return;
    if (value === undefined || value === null) {
      el.textContent = '--';
    } else {
      el.textContent = Number(value).toFixed(2);
    }
  }

  setText(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  setConnectionStatus(connected, message) {
    const el = document.getElementById('status');
    if (!el) return;
    const text = el.querySelector('.status-text');
    if (text) text.textContent = message;
    el.classList.toggle('connected', connected);
  }

  updateSensorStatus(data) {
    const dhtOk = data.dht_valid !== false && data.temperature !== -999;
    const mqOk = data.mq_valid !== false;
    this.setStatusText('dhtStatus', dhtOk);
    this.setStatusText('mqStatus', mqOk);
  }

  setStatusText(id, ok) {
    const el = document.getElementById(id);
    if (!el) return;
    el.textContent = ok ? 'норма' : 'нет данных';
    el.style.color = ok ? '#16a34a' : '#dc2626';
  }

  updateDust(data) {
    const has = data.pm1_0 !== undefined && data.pm1_0 !== null;
    this.updateRawValue('pm1_0', data.pm1_0);
    this.updateRawValue('pm2_5', data.pm2_5);
    this.updateRawValue('pm10', data.pm10);

    const text = document.getElementById('pmsStatusText');
    const info = document.getElementById('pmsStatus');
    if (text) text.textContent = has ? 'норма' : 'прогрев';
    if (info) {
      info.textContent = has ? 'норма' : 'прогрев';
      info.style.color = has ? '#16a34a' : '#d97706';
    }

    this.setBar('pm1Bar', data.pm1_0, 100);
    this.setBar('pm25Bar', data.pm2_5, 250);
    this.setBar('pm10Bar', data.pm10, 500);

    const qual = document.getElementById('pm25Quality');
    if (qual && has) {
      const v = data.pm2_5;
      const [label, color] =
        v <= 12 ? ['отличный', '#16a34a'] :
        v <= 35 ? ['хороший', '#d97706'] :
        v <= 55 ? ['умеренный', '#e67e22'] :
        v <= 150 ? ['нездоровый', '#dc2626'] :
                   ['опасный', '#7c3aed'];
      qual.textContent = label;
      qual.style.color = color;
    }

    const aqi = this.calcAQI(data.pm2_5);
    const aqiEl = document.getElementById('aqiValue');
    const aqiLbl = document.getElementById('aqiLabel');
    if (aqiEl && has) {
      aqiEl.textContent = aqi;
      const [lbl, col] =
        aqi <= 50 ? ['хорошо', '#16a34a'] :
        aqi <= 100 ? ['умеренно', '#d97706'] :
        aqi <= 150 ? ['чувствительным', '#e67e22'] :
        aqi <= 200 ? ['нездоровый', '#dc2626'] :
                     ['опасный', '#7c3aed'];
      aqiEl.style.color = col;
      if (aqiLbl) { aqiLbl.textContent = lbl; aqiLbl.style.color = col; }
    }
  }

  calcAQI(pm25) {
    if (pm25 == null) return '--';
    const bp = [[0,12,0,50],[12.1,35.4,51,100],[35.5,55.4,101,150],[55.5,150.4,151,200],[150.5,250.4,201,300]];
    for (const [cLo,cHi,iLo,iHi] of bp)
      if (pm25 >= cLo && pm25 <= cHi)
        return Math.round(((iHi-iLo)/(cHi-cLo))*(pm25-cLo)+iLo);
    return pm25 > 250 ? 301 : 0;
  }

  setBar(id, value, max) {
    const el = document.getElementById(id);
    if (!el || value == null) return;
    const pct = Math.min(100, (value / max) * 100);
    el.style.width = `${pct}%`;
    el.style.background = pct < 30 ? '#16a34a' : pct < 60 ? '#d97706' : '#dc2626';
  }

  updateRawValue(id, value) {
    const el = document.getElementById(id);
    if (el) el.textContent = (value != null) ? value : '--';
  }

  setupEventListeners() {
    const refresh = document.getElementById('refreshBtn');
    if (refresh) refresh.addEventListener('click', () => this.fetchData());

    const pause = document.getElementById('pauseBtn');
    if (pause) pause.addEventListener('click', () => {
      this.isPaused = !this.isPaused;
      pause.textContent = this.isPaused ? 'Продолжить' : 'Пауза';
      pause.classList.toggle('paused', this.isPaused);
      if (!this.isPaused) {
        this.countdown = Math.floor(this.currentInterval / 1000);
        this.fetchData();
      }
    });

    const clear = document.getElementById('clearBtn');
    if (clear) clear.addEventListener('click', () => {
      Object.keys(this.history).forEach(k => this.history[k] = []);
      this.updateCharts();
      this.addLogEntry('Графики очищены');
    });

    const slider = document.getElementById('updateInterval');
    const sliderVal = document.getElementById('intervalValue');
    if (slider && sliderVal) {
      slider.addEventListener('input', e => sliderVal.textContent = e.target.value);
      slider.addEventListener('change', e => this.updatePollingInterval(parseInt(e.target.value)));
    }

    const auto = document.getElementById('autoRefresh');
    if (auto) auto.addEventListener('change', e => {
      if (!e.target.checked) {
        if (this.updateInterval) { clearInterval(this.updateInterval); this.updateInterval = null; }
        if (this.countdownInterval) { clearInterval(this.countdownInterval); this.countdownInterval = null; }
        this.setText('nextUpdate', 'выкл');
        this.addLogEntry('Автообновление отключено');
      } else {
        this.startPolling();
        this.addLogEntry('Автообновление включено');
      }
    });

    const clearLog = document.getElementById('clearLog');
    if (clearLog) clearLog.addEventListener('click', () => {
      const log = document.getElementById('eventLog');
      if (log) log.innerHTML = `<div class="log-entry"><span class="log-time">[${new Date().toLocaleTimeString()}]</span> Журнал очищен</div>`;
    });
  }

  addLogEntry(message) {
    const log = document.getElementById('eventLog');
    if (!log) return;
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString()}]</span> ${message}`;
    log.appendChild(entry);
    log.scrollTop = log.scrollHeight;
    while (log.children.length > 50) log.removeChild(log.firstChild);
  }
}

window.addEventListener('load', () => {
  window.sensorMonitor = new SensorMonitor();
});