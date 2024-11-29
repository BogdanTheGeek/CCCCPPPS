//------------------------------------------------------------------------------
//       Filename: script.js
//------------------------------------------------------------------------------
//       Bogdan Ionescu (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Power Supply Control
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------

const expectedProductName = "CCCCPPPS";
const filter = { vendorId: 0x1209, productId: 0xd003 };

// Enable mock data for testing
var MOCK = false;
const BOOST_REPORT_SIZE = 8 * 1;
var STATS = true;

//------------------------------------------------------------------------------
// Module type definitions
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Module global variables
//------------------------------------------------------------------------------

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
    disconnected: 0
}

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

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------

/**
 * @brief  Generate samples for the plot
 * @param {number} count: Number of samples
 * @return {object} {x: number[], y: number[]}
 */
function generateSamples(count) {
    const x = new Array(count);
    const y = new Array(count);
    for (let i = 0; i < count; i++) {
        x[i] = i / samplerate;
        y[i] = 0;
    }
    return { x, y };
}

/**
 * @brief  On load event handler
 * @param  None
 * @return None
 */
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
            document.getElementById("StatusPerf").innerHTML = `Good: ${stats.good} Bad: ${stats.bad} Total: ${stats.total} Disconnected: ${stats.disconnected}`
        }, 1000);
    }

}

/**
 * @brief  Try to connect to the device
 * @param  None
 * @return None
 */
function tryConnect() {
    if (!navigator.hid) {
        return;
    }

    if (!dev) {
        navigator.hid.getDevices().then((devices) => {
            if (devices.length == 0) {
                setStatusError("No devices found. Open a device.");
            }
            else {
                devices.forEach(tryOpen);
            }
        });
    }
}

/**
 * @brief Close the device tool 
 * @param  None
 * @return None
 */
async function closeDeviceTool() {
    dev = null;
    stats.disconnected++;
    setStatusError("Disconnected");
}

/**
 * @brief  Request connection to the device
 * @param  None
 * @return None
 */
function reqConnect() {
    navigator.hid.requestDevice({ filters: [filter] })
        .then(gotUSBDevice)
        .catch(setStatusError);
}

/**
 * @brief  Callback for when a device is found
 * @param {Array} result: Array of devices
 * @return None
 */
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

/**
 * @brief  Try to open the device
 * @param {object} thisDev: Device to open
 * @return None
 */
function tryOpen(thisDev) {
    thisDev.open()
        .then((result) => {
            if (result === undefined) {
                if (dev) dev.close();
                dev = thisDev;
                setStatus("Connected");
            }
            else {
                setStatusError("Error: Could not open; " + result);
            }
        })
        .catch((e) => setStatusError("Error: Could not open; " + e));
}


/**
 * @brief  Read a 32-bit unsigned integer in little-endian format
 * @param {Uint8Array} data: Data buffer
 * @param {number} offset: Offset in the buffer
 * @return {number} The 32-bit unsigned integer
 */
function readU32LE(data, offset) {
    return data[offset] + (data[offset + 1] << 8) + (data[offset + 2] << 16) + (data[offset + 3] << 24);
}

/**
 * @brief  Write a 32-bit unsigned integer in little-endian format
 * @param {Uint8Array} data: Data buffer
 * @param {number} offset: Offset in the buffer
 * @param {number} value: The 32-bit unsigned integer
 * @return {number} The 32-bit unsigned integer
 */
function writeU32LE(data, offset, value) {
    data[offset] = value & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
    data[offset + 2] = (value >> 16) & 0xFF;
    data[offset + 3] = (value >> 24) & 0xFF;
}

/**
 * @brief  Read a 16-bit unsigned integer in little-endian format
 * @param {Uint8Array} data: Data buffer
 * @param {number} offset: Offset in the buffer
 * @return {number} The 16-bit unsigned integer
 */
function readU16LE(data, offset) {
    return data[offset] + (data[offset + 1] << 8);
}

/**
 * @brief  Write a 16-bit unsigned integer in little-endian format
 * @param {Uint8Array} data: Data buffer
 * @param {number} offset: Offset in the buffer
 * @param {number} value: The 16-bit unsigned integer
 * @return None
 */
function writeU16LE(data, offset, value) {
    data[offset] = value & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
}

/**
 * @brief  Read the status of the power supply
 * @param {object} dev: Device object
 * @return {PowerSupplyState} The power supply state
 */
async function readStatus(dev) {
    const report = await dev.receiveFeatureReport(0xAA);
    if (!report || !report.buffer || !report.buffer.byteLength) {
        throw "Error reading status";
    }

    const data = new Uint8Array(report.buffer);
    const status = new PowerSupplyState(data);
    return status;
}

/**
 * @brief  Send a command to the power supply
 * @param {object} dev: Device object
 * @param {Uint8Array} command: Command buffer
 * @return None
 */
async function sendCommand(dev, command) {
    if (command.length > BOOST_REPORT_SIZE) {
        throw "Command too long";
    }
    const report = await dev.sendFeatureReport(0xAA, command);
    if (!report) {
        throw "Error sending command";
    }
}

/**
 * @brief  Set the voltage of the power supply
 * @param {number} voltage: The voltage in mV
 * @return None
 */
async function setVoltage(voltage) {
    if (dev) {
        const command = new Uint8Array(BOOST_REPORT_SIZE - 1);
        command[0] = 1;
        writeU32LE(command, 1, voltage);
        await dev.sendFeatureReport(0xAA, command);
    }
}

/**
 * @brief  Set the current of the power supply
 * @param {number} current: The current in mA
 * @return None
 */
async function setCurrent(current) {
    if (dev) {
        const command = new Uint8Array(BOOST_REPORT_SIZE - 1);
        command[0] = 2;
        writeU32LE(command, 1, current);
        await dev.sendFeatureReport(0xAA, command);
    }
}

/**
 * @brief  Request the status of the power supply
 * @param  None
 * @return None
 */
async function requestStatus() {
    if (dev) {
        await readStatus(dev)
            .then((status) => {
                updateReadings(status)
                stats.good++;
            })
            .catch(() => {
                stats.bad++;
            });
        stats.total++;
    }
    else {
        tryConnect();
    }
}

/**
 * @brief  Resize event handler
 * @param  None
 * @return None
 */
function onResize() {
    const tab = document.getElementById("TabLive");
    tab.style.height = window.innerHeight - tab.offsetTop - 10 + "px";
}

/**
 * @brief  Set the status message
 * @param {string} msg: The message
 * @param {string} color: The color of
 * @return None
 */
function setStatus(msg, color = "green") {
    document.getElementById("status").innerHTML = `<font color=${color}>${msg}</font>`;
}

/**
 * @brief  Set the status message to error
 * @param {string} msg: The message
 * @return None
 */
function setStatusError(msg) {
    setStatus(msg, "red");
    console.trace();
}

/**
 * @brief  Update the layout of the plot
 * @param  None
 * @return None
 */
function updateLayout() {

    Plotly.relayout('canvas', {
        xaxis: {
            range: [Math.max(samplecount - samplewindow, 0), samplecount]
        },
    });
}

/**
 * @brief  Add data to the plot
 * @param {PowerSupplyState} status: The power supply status
 * @return None
 */
function addData(status) {
    if (update == false) return;
    samplecount += 1 / samplerate;

    traces[0].x.push(samplecount);
    traces[0].y.push(status.voltage);
    traces[1].x.push(samplecount);
    traces[1].y.push(status.current);
    traces[2].x.push(samplecount);
    traces[2].y.push(status.power);

    updateLayout();
}

/**
 * @brief  Switch tabs
 * @param {Event} event: The event
 * @param {string} newTabName: The new tab name
 * @return None
 */
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

/**
 * @brief  Start/Stop live update
 * @param  None
 * @return None
 */
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

/**
 * @brief  Set the sample rate
 * @param  None
 * @return None
 */
function setSampleRate() {
    clearInterval(interval);
    samplerate = document.getElementById("samplerate").value;
    if (MOCK) {
        interval = setInterval(function() {
            const voltage = Math.random() * 1000;
            const current = Math.random() * 100;
            const power = voltage * current / 1000;
            addData(new PowerSupplyState({ voltage, current, power }));
        }, 1000 / samplerate);
    }
    else {
        interval = setInterval(requestStatus, 1000 / samplerate);
    }

    samplewindow = document.getElementById("samplewindow").value;
    console.log(`Setting sample rate to ${samplerate}Hz and sample window to ${samplewindow}s`);
}

/**
 * @brief  Set the line shape
 * @param  {boolean} smooth: True for spline, false for linear
 * @return None
 */
function setLineShape(smooth) {
    const shape = smooth ? 'spline' : 'linear';
    for (let trace of traces) {
        trace.line.shape = shape;
    }
    updateLayout();
}


/**
 * @brief  Update the readings on the page
 * @param {PowerSupplyState} status: The power supply status
 * @return None
 */
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

/**
 * @brief  Send the voltage to the power supply
 * @param  None
 * @return None
 */
function sendVoltage() {
    const v = document.getElementById("SetVoltage").value;
    console.log(`Setting voltage to ${v}`);
    setVoltage(parseInt(v));
}

/**
 * @brief  Send the current to the power supply
 * @param  None
 * @return None
 */
function sendCurrent() {
    const c = document.getElementById("SetCurrent").value;
    console.log(`Setting current to ${c}`);
    setCurrent(parseInt(c));
}


