

const expectedProductName = "CCCCPPPS";
const filter = { vendorId: 0x1209, productId: 0xd003 };
let dev = null;

var update = false;
var interval;
var samplerate = 100;


var chartConfig = {
   type: 'line',
   data: {
      labels: [],
      datasets: [{
         label: 'Voltage',
         backgroundColor: "#4934ec",
         borderColor: "#4934ec",
         data: [],
         fill: false
      }, {
         label: 'Current',
         backgroundColor: "#ecc734",
         borderColor: "#ecc734",
         data: [],
         fill: false
      }, {
         label: 'Power',
         backgroundColor: "#ec6b34",
         borderColor: "#ec6b34",
         data: [],
         fill: false
      }]
   },
   options: {
      responsive: true,
      aspectRatio: 3,
      // animationEnabled: false, 
      title: {
         display: false
      },
      legend: {
         position: 'bottom',
         labels: {
            fontSize: 16,
            fontColor: '#000'
         }
      },
      tooltips: {
         mode: 'index',
         intersect: false
      },
      hover: {
         mode: 'nearest',
         intersect: true
      },
      scales: {
         xAxes: [{
            display: false,
            scaleLabel: {
               display: true,
               labelString: 'T'
            }
         }],
         yAxes: [
            {
               display: true,
               scaleLabel: {
                  display: true,
                  labelString: 'mV/mA/mW'
               }
            }
         ]
      }
   }
};


function onLoad() {
   var ctx = document.getElementById('canvas').getContext('2d');
   window.myLine = new Chart(ctx, chartConfig);
   var c = chartConfig.data.datasets;
   for (var i = 0; i < 100; ++i) {
      chartConfig.data.labels.push(" ");
      c[0].data.push(0);
      c[1].data.push(0);
      c[2].data.push(0);

   }
   updateReadings(0, 0, 0);

   if (!navigator.hid) {
      setStatusError("Browser does not support HID.");
      document.getElementById("connectButton").hidden = true;
   }
   else {
      navigator.hid.addEventListener("disconnect", (event) => { if (event.device.productName == expectedProductName) closeDeviceTool(); });
   }

   setSampleRate();

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
         setStatus("Connected.");
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

class PowerSupplyState {
   constructor(data) {
      this.voltage = readU32LE(data, 0);
      this.current = readU32LE(data, 4);
      this.power = readU32LE(data, 8);
      this.duty = data[12];
      this.ccMode = data[13] == 1;
   }
}
async function readStatus(dev) {
   const report = await dev.receiveFeatureReport(0xAA);
   if (!report) {
      throw "Error reading status";
   }

   const data = new Uint8Array(report.buffer);
   const status = new PowerSupplyState(data);
   return status;
}

async function sendCommand(dev, command) {
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
         updateReadings(status.voltage, status.current, status.power);
      }).catch((e) => {
         handleError(e);
      });
   }
   else {
      tryConnect();
   }
}


// ---------------------- UI ----------------------

function setStatus(msg) {
   document.getElementById("STATUS").innerHTML = msg;
}

function setStatusError(msg) {
   setStatus("<FONT COLOR=RED>" + msg + "</FONT>");
   console.trace();
}


function addData(voltage, current, power) {
   if (update == false) return;
   if (chartConfig.data.datasets.length > 0) {

      var c = chartConfig.data.datasets;

      c[0].data.push(voltage);
      c[1].data.push(current);
      c[2].data.push(power);

      c[0].data.shift();
      c[1].data.shift();
      c[2].data.shift();

      window.myLine.update();
   }
}

function switchTab(evt, t) {
   var i, tc, tl;
   tc = document.getElementsByClassName("tc");
   for (i = 0; i < tc.length; i++) {
      tc[i].style.display = "none";
   }
   tl = document.getElementsByClassName("tl");
   for (i = 0; i < tl.length; i++) {
      tl[i].className = tl[i].className.replace(" active", "");
   }
   document.getElementById(t).style.display = "flex";
   evt.currentTarget.className += " active";
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
   samplerate = 1000 / document.getElementById("samplerate").value;
   interval = setInterval(requestStatus, samplerate);
}

function updateReadings(voltage, current, power) {
   document.getElementById("VoltageInfo").innerHTML = "Voltage: " + voltage + "mV";
   document.getElementById("CurrentInfo").innerHTML = "Current: " + current + 'mA';
   document.getElementById("PowerInfo").innerHTML = "Power: " + power + "mW";

   document.getElementById("VoltageBig").innerHTML = voltage;
   document.getElementById("CurrentBig").innerHTML = current;
   document.getElementById("PowerBig").innerHTML = power;

   addData(voltage, current, power);
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


