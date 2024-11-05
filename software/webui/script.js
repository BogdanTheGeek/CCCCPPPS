

const expectedProductName = "CCCCPPPS";
const filter = { vendorId: 0x1209, productId: 0xd003 };
let dev = null;

var update = false;
var interval;
var samplerate = 5;
var samplewindow = 5;
var samplecount = samplewindow;

var stats = {
   good: 0,
   bad: 0,
   total: 0,
}

// Enable mock data for testing
const MOCK = false;
const BOOST_REPORT_SIZE = 8 * 3;
const STATS = true;

var traces = [{
   name: 'Voltage',
   ...generateSamples(samplewindow * samplerate),
   mode: 'lines',
   line: { color: '#4934ec', shape: 'spline' }
}, {
   name: 'Current',
   ...generateSamples(samplewindow * samplerate),
   mode: 'lines',
   line: { color: '#ecc734', shape: 'spline' }
}, {
   name: 'Power',
   ...generateSamples(samplewindow * samplerate),
   mode: 'lines',
   line: { color: '#ec6b34', shape: 'spline' }
}];

function generateSamples(count) {
   const x = new Array(count);
   const y = new Array(count);
   for (let i = 0; i < count; i++) {
      x[i] = i / samplerate;
      y[i] = 0;
   }
   return { x, y };
}

function onLoad() {

   onResize();
   updateReadings(new PowerSupplyState({}));

   const layout = {
      autosize: true,
      margin: { t: 20, b: 20, l: 40, r: 0 },
      legend: { x: 0, y: 1, traceorder: 'normal', font: { size: 16 } },
      yaxis: { automargin: true },
   }
   const config = { responsive: true }
   Plotly.newPlot('canvas', traces, layout, config);


   if (!navigator.hid) {
      setStatusError("Browser does not support HID.");
      document.getElementById("connectButton").hidden = true;
   }
   else {
      navigator.hid.addEventListener("disconnect", (event) => { if (event.device.productName == expectedProductName) closeDeviceTool(); });
   }

   const samplerate_value = document.getElementById("samplerate-value");
   document.getElementById("samplerate").oninput = function() {
      samplerate_value.innerHTML = this.value + "Hz";
   }

   const samplewindow_value = document.getElementById("samplewindow-value");
   document.getElementById("samplewindow").oninput = function() {
      samplewindow_value.innerHTML = this.value + "s";
   }

   setSampleRate();

   if (STATS) {
      setInterval(() => {
         document.getElementById("StatusPerf").innerHTML = `Good: ${stats.good} Bad: ${stats.bad} Total: ${stats.total}`;
      }, 1000);
   }

}

// ---------------------- USB ----------------------

function tryConnect() {
   if (!navigator.hid) {
      return;
   }

   if (!dev) {
      navigator.hid.getDevices().then((devices) => {
         if (devices.length == 0)
            setStatusError("No devices found. Open a device.");
         else
            devices.forEach(tryOpen);
      });
   }
}

async function closeDeviceTool() {
   setStatusError("Disconnected");
}

function reqConnect() {
   const initialization = navigator.hid.requestDevice({ filters: [filter] });
   initialization.then(gotUSBDevice);
   initialization.catch(setStatusError);
}

function gotUSBDevice(result) {
   if (result.length < 1) {
      setStatusError("Error: No devices found");
      return;
   }

   if (result[0].productName != expectedProductName) {
      setStatusError("Error: Wrong device name.  Got " + result[0].productName + " Expected " + expectedProductName);
      return;
   }

   const thisDev = result[0];

   tryOpen(thisDev);
}

function tryOpen(thisDev) {
   thisDev.open().then((result) => {
      if (result === undefined) {
         if (dev) dev.close();
         dev = thisDev;
         setStatus("Connected");
      }
      else {
         setStatusError("Error: Could not open; " + result);
      }
   }).catch((e) => setStatusError("Error: Could not open; " + e));
}

async function handleError(e) {
   if (dev) await dev.close();
   dev = null;
   setStatusError(e);
}


// -------------------- Power Supply --------------------

function readU32LE(data, offset) {
   return data[offset] + (data[offset + 1] << 8) + (data[offset + 2] << 16) + (data[offset + 3] << 24);
}

function writeU32LE(data, offset, value) {
   data[offset] = value & 0xFF;
   data[offset + 1] = (value >> 8) & 0xFF;
   data[offset + 2] = (value >> 16) & 0xFF;
   data[offset + 3] = (value >> 24) & 0xFF;
}

function readU16LE(data, offset) {
   return data[offset] + (data[offset + 1] << 8);
}

function writeU16LE(data, offset, value) {
   data[offset] = value & 0xFF;
   data[offset + 1] = (value >> 8) & 0xFF;
}

class PowerSupplyState {
   constructor(data) {
      if (data instanceof Uint8Array) {
         if (data.length < 6) {
            console.log(`Invalid data length: ${data.length}`);
         }
         this.voltage = readU16LE(data, 0);
         this.current = readU16LE(data, 2);
         if (this.voltage == 0 || this.current == 0) {
            this.power = 0;
         }
         else {
            this.power = this.voltage * this.current / 1000;
         }
         this.duty = data[4];
         this.ccMode = data[5] == 1;
      }
      else {
         this.voltage = data.voltage || 0;
         this.current = data.current || 0;
         this.power = data.power || 0;
         this.duty = data.duty || 0;
         this.ccMode = data.ccMode || false;
         return;
      }
   }
}
async function readStatus(dev) {
   const report = await dev.receiveFeatureReport(0xAA);
   if (!report || !report.buffer || !report.buffer.byteLength) {
      throw "Error reading status";
   }

   const data = new Uint8Array(report.buffer);
   const status = new PowerSupplyState(data);
   return status;
}

async function sendCommand(dev, command) {
   if (command.length > BOOST_REPORT_SIZE) {
      throw "Command too long";
   }
   const report = await dev.sendFeatureReport(0xAA, command);
   if (!report) {
      throw "Error sending command";
   }
}

async function setVoltage(voltage) {
   if (dev) {
      const command = new Uint8Array(4);
      writeU32LE(command, 0, voltage);
      await dev.sendFeatureReport(1, command);
   }
}
async function setCurrent(current) {
   if (dev) {
      const command = new Uint8Array(4);
      writeU32LE(command, 0, current);
      await dev.sendFeatureReport(2, command);
   }
}

async function requestStatus() {
   if (dev) {
      await readStatus(dev).then((status) => {
         updateReadings(status)
         stats.good++;
      }).catch((e) => {
         stats.bad++;
         handleError(e);
      });
      stats.total++;
   }
   else {
      tryConnect();
   }
}


// ---------------------- UI ----------------------

function onResize() {
   const tab = document.getElementById("TabLive");
   tab.style.height = window.innerHeight - tab.offsetTop - 10 + "px";
}

function setStatus(msg, color = "green") {
   document.getElementById("status").innerHTML = `<font color=${color}>${msg}</font>`;
}

function setStatusError(msg) {
   setStatus(msg, "red");
   console.trace();
}

function updateLayout() {

   Plotly.relayout('canvas', {
      xaxis: {
         range: [Math.max(samplecount - samplewindow, 0), samplecount]
      },
   });
}


function addData(status) {
   if (update == false) return;
   samplecount += 1 / samplerate;

   Plotly.extendTraces('canvas', {
      x: [[samplecount]],
      y: [[status.voltage]],
   }, [0]);

   Plotly.extendTraces('canvas', {
      x: [[samplecount]],
      y: [[status.current]],
   }, [1]);

   Plotly.extendTraces('canvas', {
      x: [[samplecount]],
      y: [[status.power]],
   }, [2]);

   updateLayout();
}

function switchTab(event, newTabName) {

   const tabLabels = document.getElementsByClassName("tl");
   for (e of tabLabels) {
      e.className = e.className.replace(" active", "");
   }

   const tabContents = document.getElementsByClassName("tc");
   for (e of tabContents) {
      e.className = e.className.replace(" active", "");
   }

   document.getElementById(newTabName).className += " active";
   event.currentTarget.className += " active";

   onResize();
   updateLayout();

}

function liveUpdate() {
   update = !update;
   var s = document.getElementById("startstop");
   if (!update) {
      s.style.backgroundColor = "green";
      s.value = "Start";
   } else {
      s.style.backgroundColor = "red";
      s.value = "STOP";
   }
}

function setSampleRate() {
   clearInterval(interval);
   samplerate = document.getElementById("samplerate").value;
   if (MOCK) {
      interval = setInterval(function() {
         const voltage = Math.random() * 1000;
         const current = Math.random() * 100;
         const power = voltage * current / 1000;
         addData({ voltage, current, power });
      }, 1000 / samplerate);
   }
   else {
      interval = setInterval(requestStatus, 1000 / samplerate);
   }

   samplewindow = document.getElementById("samplewindow").value;
   console.log(`Setting sample rate to ${samplerate}Hz and sample window to ${samplewindow}s`);
}

function setLineShape(smooth) {
   const shape = smooth ? 'spline' : 'linear';
   for (let trace of traces) {
      trace.line.shape = shape;
   }
   updateLayout();
}


function updateReadings(status) {
   document.getElementById("VoltageInfo").innerHTML = "Voltage: " + status.voltage + "mV";
   document.getElementById("CurrentInfo").innerHTML = "Current: " + status.current + 'mA';
   document.getElementById("PowerInfo").innerHTML = "Power: " + status.power + "mW";

   document.getElementById("VoltageBig").innerHTML = status.voltage;
   document.getElementById("CurrentBig").innerHTML = status.current;
   document.getElementById("PowerBig").innerHTML = status.power;
   document.getElementById("CC").className = status.ccMode ? "indicator on" : "indicator off";

   addData(status);
}

function sendVoltage() {
   const v = document.getElementById("SetVoltage").value;
   console.log(`Setting voltage to ${v}`);
   setVoltage(parseInt(v));
}

function sendCurrent() {
   const c = document.getElementById("SetCurrent").value;
   console.log(`Setting current to ${c}`);
   setCurrent(parseInt(c));
}


