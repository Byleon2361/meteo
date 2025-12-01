class SensorMonitor {
  constructor() {
    this.updateInterval = null;
    this.countdownInterval = null;
    this.countdown = 5;
    
    this.setupEventListeners();
    this.startPolling();
  }

  // Опрос сервера каждые 5 секунд
  startPolling() {
    this.fetchData(); // Первый запрос сразу
    
    this.updateInterval = setInterval(() => {
      this.fetchData();
    }, 5000);
    
    // Запускаем таймер обратного отсчета
    this.startCountdown();
  }

  // Таймер обратного отсчета
  startCountdown() {
    if (this.countdownInterval) {
      clearInterval(this.countdownInterval);
    }
    
    this.countdown = 5;
    this.updateCountdown();
    
    this.countdownInterval = setInterval(() => {
      this.countdown--;
      this.updateCountdown();
      
      if (this.countdown <= 0) {
        this.countdown = 5;
      }
    }, 1000);
  }

  updateCountdown() {
    const timerElement = document.getElementById('updateTimer');
    if (timerElement) {
      timerElement.textContent = `Следующее обновление через: ${this.countdown} сек`;
    }
  }

  // Получение данных с сервера
  async fetchData() {
    try {
      const response = await fetch('/get');
      if (!response.ok) {
        throw new Error(`HTTP ошибка: ${response.status}`);
      }
      
      const data = await response.json();
      this.updateDisplay(data);
      this.setConnectionStatus(true, 'Подключено');
      
      // Сбрасываем таймер
      this.startCountdown();
    } catch (error) {
      console.error('Ошибка получения данных:', error);
      this.setConnectionStatus(false, 'Ошибка подключения');
    }
  }

  // Обновление отображения значений
  updateDisplay(data) {
    const timestamp = new Date().toLocaleTimeString();
    
    // Обновляем все значения
    this.updateValue('temperature', data.temperature, '°C');
    this.updateValue('humidity', data.humidity, '%');
    this.updateValue('co2', data.CO2, 'ppm');
    this.updateValue('co', data.CO, 'ppm');
    this.updateValue('lpg', data.LPG, 'ppm');
    this.updateValue('nh3', data.NH3, 'ppm');
    
    // Время последнего обновления
    document.getElementById('lastUpdate').textContent = timestamp;
  }

  updateValue(elementId, value, unit) {
    const element = document.getElementById(elementId);
    if (element && value !== undefined) {
      element.textContent = value.toFixed(2);
      element.classList.add('updating');
      setTimeout(() => element.classList.remove('updating'), 500);
      
      // Цветовая индикация
      this.applyColorCoding(elementId, value);
    }
  }

  // Цветовая индикация
  applyColorCoding(sensorType, value) {
    const element = document.getElementById(sensorType);
    if (!element) return;

    element.classList.remove('level-good', 'level-moderate', 'level-bad', 'level-danger');

    const thresholds = {
      co2: { moderate: 800, bad: 1200, danger: 2000 },
      co: { moderate: 10, bad: 25, danger: 50 },
      lpg: { moderate: 500, bad: 1000, danger: 2000 },
      nh3: { moderate: 25, bad: 50, danger: 100 },
      temperature: { moderate: 28, bad: 35, danger: 40 },
      humidity: { moderate: 70, bad: 85, danger: 95 }
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

  // Установка статуса подключения
  setConnectionStatus(connected, message) {
    const statusElement = document.getElementById('status');
    statusElement.textContent = message;
    
    if (connected) {
      statusElement.className = 'status connected';
    } else {
      statusElement.className = 'status disconnected';
    }
  }

  // Настройка обработчиков событий
  setupEventListeners() {
    // Кнопка обновления данных
    document.getElementById('refreshBtn').addEventListener('click', () => {
      this.fetchData();
    });

    // Обновление по F5
    document.addEventListener('keydown', (e) => {
      if (e.key === 'F5') {
        e.preventDefault();
        this.fetchData();
      }
    });
  }
}

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
  window.sensorMonitor = new SensorMonitor();
});