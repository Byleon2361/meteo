class SensorMonitor {
  constructor() {
    this.socket = null;
    this.isConnected = false;
    this.sensorData = {
      co2: [],
      co: [],
      lpg: [],
      nh3: [],
      voltage: [],
      rsro: [],
      timestamps: []
    };
    this.maxDataPoints = 50;
    this.chart = null;

    this.initializeChart();
    this.connectWebSocket();
    this.setupEventListeners();
  }

  // Инициализация графика
  initializeChart() {
    const ctx = document.getElementById('sensorChart').getContext('2d');
    this.chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          {
            label: 'CO₂',
            data: [],
            borderColor: '#e74c3c',
            backgroundColor: 'rgba(231, 76, 60, 0.1)',
            tension: 0.4
          },
          {
            label: 'CO',
            data: [],
            borderColor: '#3498db',
            backgroundColor: 'rgba(52, 152, 219, 0.1)',
            tension: 0.4
          },
          {
            label: 'LPG',
            data: [],
            borderColor: '#f39c12',
            backgroundColor: 'rgba(243, 156, 18, 0.1)',
            tension: 0.4
          },
          {
            label: 'NH₃',
            data: [],
            borderColor: '#9b59b6',
            backgroundColor: 'rgba(155, 89, 182, 0.1)',
            tension: 0.4
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          y: {beginAtZero: true, title: {display: true, text: 'ppm'}},
          x: {title: {display: true, text: 'Время'}}
        }
      }
    });
  }

  // Подключение к WebSocket
  connectWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    this.socket = new WebSocket(wsUrl);

    this.socket.onopen = () => {
      this.setConnectionStatus(true, 'Подключено');
      console.log('WebSocket подключен');
    };

    this.socket.onclose = () => {
      this.setConnectionStatus(false, 'Отключено');
      console.log('WebSocket отключен');
      // Попытка переподключения через 5 секунд
      setTimeout(() => this.connectWebSocket(), 5000);
    };

    this.socket.onmessage = (event) => {
      this.handleSensorData(JSON.parse(event.data));
    };

    this.socket.onerror = (error) => {
      console.error('WebSocket ошибка:', error);
      this.setConnectionStatus(false, 'Ошибка подключения');
    };
  }

  // Обработка данных с датчиков
  handleSensorData(data) {
    const timestamp = new Date().toLocaleTimeString();

    // Обновление значений на дисплее
    this.updateDisplay('co2', data.co2_ppm);
    this.updateDisplay('co', data.co_ppm);
    this.updateDisplay('lpg', data.lpg_ppm);
    this.updateDisplay('nh3', data.nh3_ppm);
    this.updateDisplay('voltage', data.voltage);
    this.updateDisplay('rsro', data.rs_ro_ratio);
    this.updateDisplay('lastUpdate', timestamp);

    // Добавление данных в историю
    this.addToHistory(data, timestamp);

    // Обновление графика
    this.updateChart();

    // Проверка предупреждений
    this.checkAlerts(data);
  }

  // Обновление отображения значений
  updateDisplay(elementId, value) {
    const element = document.getElementById(elementId);
    if (element) {
      element.textContent =
          typeof value === 'number' ? value.toFixed(2) : value;
      element.classList.add('updating');
      setTimeout(() => element.classList.remove('updating'), 1000);

      // Добавление цветового кодирования для значений
      this.applyValueStyling(elementId, value);
    }
  }

  // Применение стилей в зависимости от значения
  applyValueStyling(sensorType, value) {
    const element = document.getElementById(sensorType);
    if (!element) return;

    element.classList.remove(
        'level-good', 'level-moderate', 'level-bad', 'level-danger');

    const thresholds = {
      co2: {moderate: 800, bad: 1200, danger: 2000},
      co: {moderate: 10, bad: 25, danger: 50},
      lpg: {moderate: 500, bad: 1000, danger: 2000},
      nh3: {moderate: 25, bad: 50, danger: 100}
    };

    const sensorThresholds = thresholds[sensorType];
    if (sensorThresholds) {
      if (value >= sensorThresholds.danger) {
        element.classList.add('level-danger');
      } else if (value >= sensorThresholds.bad) {
        element.classList.add('level-bad');
      } else if (value >= sensorThresholds.moderate) {
        element.classList.add('level-moderate');
      } else {
        element.classList.add('level-good');
      }
    }
  }

  // Добавление данных в историю
  addToHistory(data, timestamp) {
    this.sensorData.co2.push(data.co2_ppm);
    this.sensorData.co.push(data.co_ppm);
    this.sensorData.lpg.push(data.lpg_ppm);
    this.sensorData.nh3.push(data.nh3_ppm);
    this.sensorData.voltage.push(data.voltage);
    this.sensorData.rsro.push(data.rs_ro_ratio);
    this.sensorData.timestamps.push(timestamp);

    // Ограничение количества точек данных
    if (this.sensorData.co2.length > this.maxDataPoints) {
      this.sensorData.co2.shift();
      this.sensorData.co.shift();
      this.sensorData.lpg.shift();
      this.sensorData.nh3.shift();
      this.sensorData.voltage.shift();
      this.sensorData.rsro.shift();
      this.sensorData.timestamps.shift();
    }
  }

  // Обновление графика
  updateChart() {
    this.chart.data.labels = this.sensorData.timestamps;
    this.chart.data.datasets[0].data = this.sensorData.co2;
    this.chart.data.datasets[1].data = this.sensorData.co;
    this.chart.data.datasets[2].data = this.sensorData.lpg;
    this.chart.data.datasets[3].data = this.sensorData.nh3;
    this.chart.update();
  }

  // Проверка предупреждений
  checkAlerts(data) {
    const alerts = [];

    if (data.co_ppm > 50) alerts.push('ВЫСОКИЙ УРОВЕНЬ CO!');
    if (data.co2_ppm > 2000) alerts.push('ВЫСОКИЙ УРОВЕНЬ CO₂!');
    if (data.lpg_ppm > 2000) alerts.push('ОБНАРУЖЕН ГАЗ!');

    if (alerts.length > 0) {
      this.showAlert(alerts.join('\n'));
    }
  }

  // Показ предупреждения
  showAlert(message) {
    // В реальном приложении здесь может быть модальное окно или уведомление
    console.warn('ПРЕДУПРЕЖДЕНИЕ:', message);

    // Визуальное мигание статуса
    const status = document.getElementById('status');
    status.style.background = '#e74c3c';
    setTimeout(() => {
      if (this.isConnected) {
        status.style.background = '#27ae60';
      }
    }, 2000);
  }

  // Установка статуса подключения
  setConnectionStatus(connected, message) {
    this.isConnected = connected;
    const statusElement = document.getElementById('status');
    statusElement.textContent = message;
    statusElement.className =
        connected ? 'status connected' : 'status disconnected';
  }

  // Настройка обработчиков событий
  setupEventListeners() {
    // Обработчик обновления страницы
    window.addEventListener('beforeunload', () => {
      if (this.socket) {
        this.socket.close();
      }
    });

    // Кнопка ручного обновления (можно добавить в HTML)
    document.addEventListener('keydown', (e) => {
      if (e.key === 'r' && e.ctrlKey) {
        location.reload();
      }
    });
  }
}

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
  window.sensorMonitor = new SensorMonitor();
});

// Фолбэк для демонстрации (если WebSocket недоступен)
function startDemoMode() {
  console.log('Запуск демо-режима');

  setInterval(() => {
    const demoData = {
      co2_ppm: 400 + Math.random() * 100,
      co_ppm: 1 + Math.random() * 5,
      lpg_ppm: 10 + Math.random() * 20,
      nh3_ppm: 2 + Math.random() * 8,
      voltage: 2.0 + Math.random() * 0.5,
      rs_ro_ratio: 8 + Math.random() * 4
    };

    if (window.sensorMonitor) {
      window.sensorMonitor.handleSensorData(demoData);
    }
  }, 2000);
}

// Автозапуск демо-режима через 10 секунд если нет подключения
setTimeout(() => {
  if (!window.sensorMonitor || !window.sensorMonitor.isConnected) {
    startDemoMode();
    document.getElementById('status').textContent = 'Демо-режим';
  }
}, 10000);