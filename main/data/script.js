class SensorMonitor {
  constructor() {
    this.updateInterval = null;
    this.countdown = 5;
    this.currentInterval = 5000; // Текущий интервал в миллисекундах
    this.isPaused = false;
    this.updateCount = 0;
    this.startTime = Date.now();
    this.relayState = false; 
    
    // Графики на чистом Canvas
    this.tempHumChart = null;
    this.gasChart = null;
    this.dustChart = null;
    
    // Данные для истории
    this.history = {
      timestamps: [],
      temperature: [],
      humidity: [],
      co2: [],
      co: [],
      lpg: [],
      nh3: [],
      pm1_0: [],
      pm2_5: [],
      pm10:  []
    };
    this.maxHistoryPoints = 20;
    
    this.initialize();
  }

  initialize() {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', () => {
        this.setupCharts();
        this.setupEventListeners();
        this.startPolling();
        this.startUptimeCounter();
      });
    } else {
      this.setupCharts();
      this.setupEventListeners();
      this.startPolling();
      this.startUptimeCounter();
    }
  }

  setupCharts() {
    const tempHumCanvas = document.getElementById('tempHumChart');
    const gasCanvas = document.getElementById('gasChart');
    const dustCanvas = document.getElementById('dustChart');
    
    if (tempHumCanvas) {
      this.initializeTempHumChart(tempHumCanvas);
    }
    
    if (gasCanvas) {
      this.initializeGasChart(gasCanvas);
    }
    if (dustCanvas) {
      this.initializeDustChart(dustCanvas);
    }
  }

  // Простой график температуры и влажности на Canvas
  initializeTempHumChart(canvas) {
    try {
      const ctx = canvas.getContext('2d');
      this.tempHumChart = {
        ctx: ctx,
        width: canvas.width,
        height: canvas.height,
        
        draw: function(data) {
          const ctx = this.ctx;
          ctx.clearRect(0, 0, this.width, this.height);
          
          if (!data || data.timestamps.length < 2) {
            this.drawNoData();
            return;
          }
          
          // Рисуем сетку
          this.drawGrid();
          
          // Рисуем линии
          this.drawLine(data.timestamps, data.temperature, '#e74c3c', 'Температура');
          this.drawLine(data.timestamps, data.humidity, '#3498db', 'Влажность');
          
          // Легенда
          this.drawLegend();
        },
        
        drawGrid: function() {
          const ctx = this.ctx;
          ctx.strokeStyle = '#eee';
          ctx.lineWidth = 1;
          
          // Вертикальные линии
          for (let i = 0; i <= 5; i++) {
            const x = i * this.width / 5;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, this.height);
            ctx.stroke();
          }
          
          // Горизонтальные линии
          for (let i = 0; i <= 5; i++) {
            const y = i * this.height / 5;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(this.width, y);
            ctx.stroke();
          }
        },
        
        drawLine: function(labels, values, color, label) {
          if (!values || values.length < 2) return;
          
          const ctx = this.ctx;
          const max = Math.max(...values);
          const min = Math.min(...values);
          const range = max - min || 1;
          
          ctx.strokeStyle = color;
          ctx.lineWidth = 2;
          ctx.beginPath();
          
          values.forEach((value, index) => {
            const x = index * this.width / (values.length - 1);
            const y = this.height - ((value - min) / range * this.height * 0.8) - this.height * 0.1;
            
            if (index === 0) {
              ctx.moveTo(x, y);
            } else {
              ctx.lineTo(x, y);
            }
          });
          
          ctx.stroke();
        },
        
        drawLegend: function() {
          const ctx = this.ctx;
          ctx.font = '12px Arial';
          ctx.fillStyle = '#e74c3c';
          ctx.fillText('Температура', 10, 20);
          ctx.fillStyle = '#3498db';
          ctx.fillText('Влажность', 10, 40);
        },
        
        drawNoData: function() {
          const ctx = this.ctx;
          ctx.font = '16px Arial';
          ctx.fillStyle = '#95a5a6';
          ctx.textAlign = 'center';
          ctx.fillText('Нет данных для графика', this.width / 2, this.height / 2);
          ctx.textAlign = 'left';
        }
      };
    } catch (error) {
      console.error('Ошибка инициализации графика температуры:', error);
    }
  }

  // Простой график газов на Canvas
  initializeGasChart(canvas) {
    try {
      const ctx = canvas.getContext('2d');
      this.gasChart = {
        ctx: ctx,
        width: canvas.width,
        height: canvas.height,
        
        draw: function(data) {
          const ctx = this.ctx;
          ctx.clearRect(0, 0, this.width, this.height);
          
          if (!data || data.timestamps.length < 2) {
            this.drawNoData();
            return;
          }
          
          // Рисуем сетку
          this.drawGrid();
          
          // Рисуем линии газов
          const gases = [
            { data: data.co2, color: '#2ecc71', label: 'CO₂' },
            { data: data.co, color: '#f39c12', label: 'CO' },
            { data: data.lpg, color: '#9b59b6', label: 'LPG' },
            { data: data.nh3, color: '#1abc9c', label: 'NH₃' }
          ];
          
          gases.forEach(gas => {
            if (gas.data && gas.data.length > 0) {
              this.drawLine(data.timestamps, gas.data, gas.color, gas.label);
            }
          });
          
          // Легенда
          this.drawGasLegend(gases);
        },
        
        drawGrid: function() {
          const ctx = this.ctx;
          ctx.strokeStyle = '#eee';
          ctx.lineWidth = 1;
          
          // Вертикальные линии
          for (let i = 0; i <= 5; i++) {
            const x = i * this.width / 5;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, this.height);
            ctx.stroke();
          }
          
          // Горизонтальные линии
          for (let i = 0; i <= 5; i++) {
            const y = i * this.height / 5;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(this.width, y);
            ctx.stroke();
          }
        },
        
        drawLine: function(labels, values, color, label) {
          if (!values || values.length < 2) return;
          
          const ctx = this.ctx;
          const max = Math.max(...values);
          const min = Math.min(...values);
          const range = max - min || 1;
          
          ctx.strokeStyle = color;
          ctx.lineWidth = 2;
          ctx.beginPath();
          
          values.forEach((value, index) => {
            const x = index * this.width / (values.length - 1);
            const y = this.height - ((value - min) / range * this.height * 0.8) - this.height * 0.1;
            
            if (index === 0) {
              ctx.moveTo(x, y);
            } else {
              ctx.lineTo(x, y);
            }
          });
          
          ctx.stroke();
        },
        
        drawGasLegend: function(gases) {
          const ctx = this.ctx;
          ctx.font = '12px Arial';
          
          gases.forEach((gas, index) => {
            ctx.fillStyle = gas.color;
            ctx.fillText(gas.label, 10, 20 + (index * 20));
          });
        },
        
        drawNoData: function() {
          const ctx = this.ctx;
          ctx.font = '16px Arial';
          ctx.fillStyle = '#95a5a6';
          ctx.textAlign = 'center';
          ctx.fillText('Нет данных для графика', this.width / 2, this.height / 2);
          ctx.textAlign = 'left';
        }
      };
    } catch (error) {
      console.error('Ошибка инициализации графика газов:', error);
    }
  }
  initializeDustChart(canvas) {
      const ctx = canvas.getContext('2d');
      this.dustChart = {
          ctx, width: canvas.width, height: canvas.height,
          draw(data) {
              this.ctx.clearRect(0, 0, this.width, this.height);
              if (!data || data.timestamps.length < 2) {
                  this.ctx.font = '16px Arial'; this.ctx.fillStyle = '#95a5a6';
                  this.ctx.textAlign = 'center';
                  this.ctx.fillText('Ожидание данных PMS5003...', this.width/2, this.height/2);
                  this.ctx.textAlign = 'left'; return;
              }
              // сетка
              this.ctx.strokeStyle = '#eee'; this.ctx.lineWidth = 1;
              for (let i = 0; i <= 5; i++) {
                  const x = i * this.width / 5, y = i * this.height / 5;
                  this.ctx.beginPath(); this.ctx.moveTo(x,0); this.ctx.lineTo(x,this.height); this.ctx.stroke();
                  this.ctx.beginPath(); this.ctx.moveTo(0,y); this.ctx.lineTo(this.width,y); this.ctx.stroke();
              }
              // линии
              [['pm1_0','#3498db','PM1.0'],['pm2_5','#e67e22','PM2.5'],['pm10','#e74c3c','PM10']].forEach(([key,color,label], i) => {
                  const values = data[key];
                  if (!values || values.length < 2) return;
                  const max = Math.max(...values), min = Math.min(...values), range = max - min || 1;
                  this.ctx.strokeStyle = color; this.ctx.lineWidth = 2; this.ctx.beginPath();
                  values.forEach((v, idx) => {
                      const x = idx * this.width / (values.length - 1);
                      const y = this.height - ((v - min) / range * this.height * 0.8) - this.height * 0.1;
                      idx === 0 ? this.ctx.moveTo(x,y) : this.ctx.lineTo(x,y);
                  });
                  this.ctx.stroke();
                  this.ctx.fillStyle = color; this.ctx.font = '12px Arial';
                  this.ctx.fillText(label, 10, 20 + i * 20);
              });
          }
      };
  }
  // Добавление данных в историю
  addToHistory(data, timestamp) {
    this.history.timestamps.push(timestamp);
    this.history.temperature.push(data.temperature || 0);
    this.history.humidity.push(data.humidity || 0);
    this.history.co2.push(data.CO2 || 0);
    this.history.co.push(data.CO || 0);
    this.history.lpg.push(data.LPG || 0);
    this.history.nh3.push(data.NH3 || 0);
    this.history.pm1_0.push(data.pm1_0 ?? 0);
    this.history.pm2_5.push(data.pm2_5 ?? 0);
    this.history.pm10.push(data.pm10   ?? 0);
    
    // Ограничиваем историю
    if (this.history.timestamps.length > this.maxHistoryPoints) {
      this.history.timestamps.shift();
      this.history.temperature.shift();
      this.history.humidity.shift();
      this.history.co2.shift();
      this.history.co.shift();
      this.history.lpg.shift();
      this.history.nh3.shift();
      this.history.pm1_0.shift();
      this.history.pm2_5.shift();
      this.history.pm10.shift();
    }
    
    // Обновляем графики
    this.updateCharts();
  }

  // Обновление графиков
  updateCharts() {
    if (this.tempHumChart && this.tempHumChart.draw) {
      this.tempHumChart.draw(this.history);
    }
    
    if (this.gasChart && this.gasChart.draw) {
      this.gasChart.draw(this.history);
    }

    if (this.dustChart && this.dustChart.draw) {
      this.dustChart.draw(this.history);
    }
  }

  // Опрос сервера
  startPolling() {
    this.fetchData();
    
    // Очищаем предыдущий интервал, если он есть
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
    }
    
    this.updateInterval = setInterval(() => {
      if (!this.isPaused) {
        this.fetchData();
      }
    }, this.currentInterval);
    
    this.startCountdown();
  }

  // Обновление интервала опроса
  updatePollingInterval(newIntervalSeconds) {
    const newIntervalMs = newIntervalSeconds * 1000;
    this.currentInterval = newIntervalMs;
    
    // Обновляем обратный отсчет
    this.countdown = newIntervalSeconds;
    
    // Очищаем старый интервал
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
    }
    
    // Создаем новый интервал
    this.updateInterval = setInterval(() => {
      if (!this.isPaused) {
        this.fetchData();
      }
    }, newIntervalMs);
    
    // Немедленно обновляем отображение следующего обновления
    const nextUpdateEl = document.getElementById('nextUpdate');
    if (nextUpdateEl) {
      nextUpdateEl.textContent = `${this.countdown} сек`;
    }
    
    console.log(`Интервал обновления изменен на ${newIntervalSeconds} секунд`);
    this.addLogEntry('info', `Интервал обновления изменен на ${newIntervalSeconds} секунд`);
  }

  startCountdown() {
    // Очищаем предыдущий счетчик, если он существует
    if (this.countdownInterval) {
      clearInterval(this.countdownInterval);
    }
    
    this.countdownInterval = setInterval(() => {
      if (!this.isPaused) {
        this.countdown--;
        if (this.countdown <= 0) {
          // Сбрасываем счетчик до текущего интервала в секундах
          this.countdown = Math.floor(this.currentInterval / 1000);
        }
        const nextUpdateEl = document.getElementById('nextUpdate');
        if (nextUpdateEl) {
          nextUpdateEl.textContent = `${this.countdown} сек`;
        }
      }
    }, 1000);
  }

  startUptimeCounter() {
    if (this.uptimeInterval) {
      clearInterval(this.uptimeInterval);
    }
    
    this.uptimeInterval = setInterval(() => {
      const uptimeSeconds = Math.floor((Date.now() - this.startTime) / 1000);
      const hours = Math.floor(uptimeSeconds / 3600);
      const minutes = Math.floor((uptimeSeconds % 3600) / 60);
      const seconds = uptimeSeconds % 60;
      
      const uptimeString = `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
      const uptimeEl = document.getElementById('uptime');
      if (uptimeEl) {
        uptimeEl.textContent = uptimeString;
      }
    }, 1000);
  }

  async fetchData() {
    try {
      console.log('Запрос данных с /get...');
      
      const response = await fetch('/get', {
        headers: {
          'Cache-Control': 'no-cache'
        }
      });
      
      if (!response.ok) {
        throw new Error(`HTTP ошибка: ${response.status}`);
      }
      
      const data = await response.json();
      console.log('Получены данные:', data);
      
      this.updateDisplay(data);
      this.updateCount++;
      
      // Добавляем в историю
      const timestamp = new Date().toLocaleTimeString();
      this.addToHistory(data, timestamp);
      
      this.setConnectionStatus(true, 'Подключено');
      
      const totalUpdatesEl = document.getElementById('totalUpdates');
      if (totalUpdatesEl) {
        totalUpdatesEl.textContent = this.updateCount;
      }
      
    } catch (error) {
      console.error('Ошибка получения данных:', error);
      this.setConnectionStatus(false, 'Ошибка соединения');
    }
  }

  updateDisplay(data) {
    const timestamp = new Date().toLocaleTimeString();
    
    // ОТЛАДКА: выводим в консоль перед обновлением
    console.log('Обновление данных на странице:', data);
    
    // Обновляем значения с проверкой
    this.updateValue('temperature', data.temperature);
    this.updateValue('humidity', data.humidity);
    this.updateValue('co2', data.CO2);
    this.updateValue('co', data.CO);
    this.updateValue('lpg', data.LPG);
    this.updateValue('nh3', data.NH3);
    this.updateDust(data)
    
    // Время обновления
    const lastUpdateEl = document.getElementById('lastUpdate');
    if (lastUpdateEl) {
      console.log('Обновление времени:', timestamp);
      lastUpdateEl.textContent = timestamp;
    }
    
    // Статусы датчиков
    this.updateSensorStatus(data);
    
    // Визуальные индикаторы
    this.updateVisualIndicators(data);
  }

  updateValue(elementId, value) {
    const element = document.getElementById(elementId);
    if (element) {
      console.log(`Обновление ${elementId}:`, value);
      
      if (value === undefined || value === null) {
        element.textContent = '--';
        element.style.color = '#95a5a6';
      } else {
        element.textContent = value.toFixed(2);
        element.style.color = '';
      }
    } else {
      console.error(`Элемент ${elementId} не найден!`);
    }
  }

  setConnectionStatus(connected, message) {
    const statusElement = document.getElementById('status');
    if (statusElement) {
      const statusText = statusElement.querySelector('.status-text');
      const statusIndicator = statusElement.querySelector('.status-indicator');
      
      if (statusText) statusText.textContent = message;
      if (statusIndicator) {
        statusIndicator.style.background = connected ? '#27ae60' : '#e74c3c';
      }
      
      statusElement.className = connected ? 'status connected' : 'status disconnected';
    }
  }

  updateSensorStatus(data) {
    const dhtValid = data.dht_valid !== false && data.temperature !== -999;
    const mqValid = data.mq_valid !== false;
    
    const dhtStatusEl = document.getElementById('dhtStatus');
    const mqStatusEl = document.getElementById('mqStatus');
    
    if (dhtStatusEl) {
      dhtStatusEl.textContent = dhtValid ? '✅ Работает' : '❌ Ошибка';
      dhtStatusEl.style.color = dhtValid ? '#27ae60' : '#e74c3c';
    }
    
    if (mqStatusEl) {
      mqStatusEl.textContent = mqValid ? '✅ Работает' : '❌ Ошибка';
      mqStatusEl.style.color = mqValid ? '#27ae60' : '#e74c3c';
    }
  }

  updateVisualIndicators(data) {
    // Индикатор температуры
    const tempValue = data.temperature || 0;
    const tempPercent = ((tempValue + 20) / 80) * 100;
    const tempRange = document.getElementById('tempRange');
    if (tempRange) {
      tempRange.style.width = `${Math.min(100, Math.max(0, tempPercent))}%`;
      tempRange.style.background = tempValue > 30 ? '#e74c3c' : tempValue > 25 ? '#f39c12' : '#2ecc71';
    }
    
    // Индикатор влажности
    const humValue = data.humidity || 0;
    const humRange = document.getElementById('humRange');
    if (humRange) {
      humRange.style.width = `${humValue}%`;
      humRange.style.background = humValue > 80 ? '#e74c3c' : humValue > 60 ? '#f39c12' : '#2ecc71';
    }
  }

  setupEventListeners() {
    // Кнопка обновления
    const refreshBtn = document.getElementById('refreshBtn');
    if (refreshBtn) {
      refreshBtn.addEventListener('click', () => this.fetchData());
    }
    
    // Кнопка паузы
    const pauseBtn = document.getElementById('pauseBtn');
    if (pauseBtn) {
      pauseBtn.addEventListener('click', () => {
        this.isPaused = !this.isPaused;
        const btnIcon = pauseBtn.querySelector('.btn-icon');
        const btnText = pauseBtn.querySelector('.btn-text');
        
        if (btnIcon) btnIcon.textContent = this.isPaused ? '▶️' : '⏸️';
        if (btnText) btnText.textContent = this.isPaused ? 'Продолжить' : 'Пауза';
        
        pauseBtn.classList.toggle('paused', this.isPaused);
        
        if (!this.isPaused) {
          // При продолжении сразу обновляем данные и сбрасываем счетчик
          this.countdown = Math.floor(this.currentInterval / 1000);
          this.fetchData();
        }
      });
    }
    
    // Кнопка очистки графиков
    const clearBtn = document.getElementById('clearBtn');
    if (clearBtn) {
      clearBtn.addEventListener('click', () => {
        this.history = {
          timestamps: [],
          temperature: [],
          humidity: [],
          co2: [],
          co: [],
          lpg: [],
          nh3: [],
          pm1_0: [],
          pm2_5: [],
          pm10: []
        };
        this.updateCharts();
        this.addLogEntry('info', 'Графики очищены');
      });
    }
    
    // Кнопка реле
    const relayBtn = document.getElementById('relayBtn');
    if (relayBtn) {
      relayBtn.addEventListener('click', () => {
        this.toggleRelay();
      });
    }
    
    // Слайдер интервала - ФИКСИРУЕМ ЭТОТ КОД
    const intervalSlider = document.getElementById('updateInterval');
    const intervalValue = document.getElementById('intervalValue');
    if (intervalSlider && intervalValue) {
      // Обновляем отображение значения при перемещении ползунка
      intervalSlider.addEventListener('input', (e) => {
        const value = e.target.value;
        intervalValue.textContent = value;
      });
      
      // При изменении интервала - обновляем систему опроса
      intervalSlider.addEventListener('change', (e) => {
        const newInterval = parseInt(e.target.value);
        this.updatePollingInterval(newInterval);
      });
    }
    
    // Чекбокс автоматического обновления
    const autoRefreshCheckbox = document.getElementById('autoRefresh');
    if (autoRefreshCheckbox) {
      autoRefreshCheckbox.addEventListener('change', (e) => {
        if (!e.target.checked) {
          // Если автоматическое обновление выключено
          if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
          }
          if (this.countdownInterval) {
            clearInterval(this.countdownInterval);
            this.countdownInterval = null;
          }
          const nextUpdateEl = document.getElementById('nextUpdate');
          if (nextUpdateEl) {
            nextUpdateEl.textContent = 'выкл';
          }
          this.addLogEntry('info', 'Автоматическое обновление отключено');
        } else {
          // Если автоматическое обновление включено
          this.startPolling();
          this.addLogEntry('info', 'Автоматическое обновление включено');
        }
      });
    }
    
    // Очистка лога
    const clearLogBtn = document.getElementById('clearLog');
    if (clearLogBtn) {
      clearLogBtn.addEventListener('click', () => {
        const eventLog = document.getElementById('eventLog');
        if (eventLog) {
          eventLog.innerHTML = '<div class="log-entry info"><span class="log-time">[' + 
            new Date().toLocaleTimeString() + ']</span><span class="log-message">Лог очищен</span></div>';
        }
      });
    }
  }

  // Переключение реле
  toggleRelay() {
    this.relayState = !this.relayState;
    this.updateRelayButton();
    this.sendRelayCommand();
    
    // Добавляем запись в лог
    const message = this.relayState ? 'Реле ВКЛЮЧЕНО' : 'Реле ВЫКЛЮЧЕНО';
    this.addLogEntry('relay', message);
    
    console.log(`Реле: ${this.relayState ? 'ВКЛ' : 'ВЫКЛ'}`);
  }

  // Обновление кнопки реле
  updateRelayButton() {
    const relayBtn = document.getElementById('relayBtn');
    if (relayBtn) {
      const btnText = relayBtn.querySelector('.btn-text');
      const btnIcon = relayBtn.querySelector('.btn-icon');
      
      if (this.relayState) {
        // Реле ВКЛ
        btnText.textContent = 'Реле: ВКЛ';
        btnIcon.textContent = '🔌'; // Иконка вилки
        relayBtn.classList.add('active');
        relayBtn.style.background = '#27ae60'; // Зеленый
      } else {
        // Реле ВЫКЛ
        btnText.textContent = 'Реле: ВЫКЛ';
        btnIcon.textContent = '⚡'; // Иконка молнии
        relayBtn.classList.remove('active');
        relayBtn.style.background = '#8e44ad'; // Фиолетовый
      }
    }
  }

  // Отправка команды на ESP32
  async sendRelayCommand() {
    try {
      const command = this.relayState ? 'on' : 'off';
      const response = await fetch(`/relay?state=${command}`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json'
        }
      });
      
      if (!response.ok) {
        throw new Error(`Ошибка HTTP: ${response.status}`);
      }
      
      const result = await response.text();
      console.log(`Ответ от реле: ${result}`);
      
    } catch (error) {
      console.error('Ошибка управления реле:', error);
      this.addLogEntry('error', `Ошибка управления реле: ${error.message}`);
    }
  }

  // Добавление записи в лог
  addLogEntry(type, message) {
    const eventLog = document.getElementById('eventLog');
    if (eventLog) {
      const logEntry = document.createElement('div');
      logEntry.className = `log-entry ${type}`;
      logEntry.innerHTML = `
        <span class="log-time">[${new Date().toLocaleTimeString()}]</span>
        <span class="log-message">${message}</span>
      `;
      eventLog.appendChild(logEntry);
      
      // Прокрутка к последней записи
      eventLog.scrollTop = eventLog.scrollHeight;
      
      // Ограничиваем количество записей (максимум 50)
      while (eventLog.children.length > 50) {
        eventLog.removeChild(eventLog.firstChild);
      }
    }
  }
updateDust(data) {
    const has = data.pm1_0 !== undefined && data.pm1_0 !== null;
    this.updateRawValue('pm1_0', data.pm1_0);
    this.updateRawValue('pm2_5', data.pm2_5);
    this.updateRawValue('pm10',  data.pm10);

    const dot  = document.querySelector('.pms-dot');
    const text = document.getElementById('pmsStatusText');
    const info = document.getElementById('pmsStatus');
    if (has) {
        if (dot)  dot.style.background = '#27ae60';
        if (text) text.textContent = 'Онлайн';
        if (info) { info.textContent = '✅ Работает'; info.style.color = '#27ae60'; }
    } else {
        if (dot)  dot.style.background = '#e74c3c';
        if (text) text.textContent = 'Нет данных';
        if (info) { info.textContent = '⏳ Прогрев...'; info.style.color = '#f39c12'; }
    }

    this.setDustBar('pm1Bar',  data.pm1_0, 100);
    this.setDustBar('pm25Bar', data.pm2_5, 250);
    this.setDustBar('pm10Bar', data.pm10,  500);

    const qualEl = document.getElementById('pm25Quality');
    if (qualEl && has) {
        const v = data.pm2_5;
        const [label, color] =
            v <= 12  ? ['🟢 Отличный',  '#27ae60'] :
            v <= 35  ? ['🟡 Хороший',   '#f39c12'] :
            v <= 55  ? ['🟠 Умеренный', '#e67e22'] :
            v <= 150 ? ['🔴 Нездоровый','#e74c3c'] :
                       ['🟣 Опасный',   '#8e44ad'];
        qualEl.textContent = label; qualEl.style.color = color;
    }

    const aqi    = this.calcAQI(data.pm2_5);
    const aqiEl  = document.getElementById('aqiValue');
    const aqiLbl = document.getElementById('aqiLabel');
    if (aqiEl && has) {
        aqiEl.textContent = aqi;
        const [lbl, col] =
            aqi <= 50  ? ['Хорошо',         '#27ae60'] :
            aqi <= 100 ? ['Умеренно',       '#f39c12'] :
            aqi <= 150 ? ['Чувствительным', '#e67e22'] :
            aqi <= 200 ? ['Нездоровый',     '#e74c3c'] :
                         ['Опасный',        '#8e44ad'];
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

setDustBar(id, value, max) {
    const el = document.getElementById(id);
    if (!el || value == null) return;
    const pct = Math.min(100, (value / max) * 100);
    el.style.width = `${pct}%`;
    el.style.background = pct < 30 ? '#27ae60' : pct < 60 ? '#f39c12' : '#e74c3c';
}

updateRawValue(id, value) {
    const el = document.getElementById(id);
    if (el) el.textContent = (value != null) ? value : '--';
}
  
}

// Запуск системы
window.addEventListener('load', () => {
  console.log('Страница загружена, инициализация системы...');
  window.sensorMonitor = new SensorMonitor();
});

