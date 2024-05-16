static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>RFM69 Gateway</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    //console.log(evt);
    // evt.data holds the gateway radio status info in JSON format
    var gwobj = JSON.parse(evt.data);
    if (gwobj && gwobj.msgType) {
      if (gwobj.msgType === "config") {
        var eConfig = document.getElementById('rfm69Config');
        eConfig.innerHTML = '<p>Frequency ' + gwobj.freq + ' MHz' +
          ', Network ID:' + gwobj.netid +
          ', RFM69HCW:' + gwobj.rfm69hcw +
          ', Power Level:' + gwobj.power;
      }
    }
  };
}
</script>
</head>
<body onload="javascript:start();">
<h2>RFM69 Gateway</h2>
<div id="rfm69Config"></div>
<div id="configDev">
  <p><a href="/configDev"><button type="button">Configure Device</button></a>
</div>
<div id="FirmwareUpdate">
  <p><a href="/updater"><button type="button">Update Device Firmware</button></a>
</div>
</body>
</html>
)rawliteral";


static const char PROGMEM CONFIGUREDEV_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
  <title>Device Configuration</title>
  <style>
    "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
  </style>
</head>
<body>
  <h3>Update Device Configuration</h3>
  <form method='POST' action='/configDev' enctype='multipart/form-data'>
    <label>RFM69 Encryption Key</label>
    <input type='text' name='encryptkey' value="%s" size="16" maxlength="16"><br>

    <label>Network ID</label>
    <input type='number' name='networkid' value="%d" min="1" max="255" size="3"><br>

    <label>Device Node ID</label>
    <input type='number' name='nodeid' value="%d" min="1" max="255" size="3"><br>

    <label>Device AP name</label>
    <input type='text' name='devapname' value="%s" size="32" maxlength="32"><br>
<!--
    <label for=hcw>RFM69 HCW</label><br>
    <input type='radio' name='rfm69hcw' id="hcw" value="1" %s> True<br>
    <input type='radio' name='rfm69hcw' id="hcw" value="0" %s> False<br>

    <label>RFM69 Power Level</label>
    <input type='number' name='powerlevel' value="%d" min="0" max="31"size="2"><br>

    <label>RFM69 Frequency</label>
    <select name="rfmfrequency">
    <option value="31" %s>315 MHz</option>
    <option value="43" %s>433 MHz</option>
    <option value="86" %s>868 MHz</option>
    <option value="91" %s>915 MHz</option>
    </select><br>
-->
    <p><input type='submit' value='Save changes'>
  </form>
  <p><a href="/configDev"><button type="button">Cancel</button></a><a href="/configReset"><button type="button">Factory Reset</button></a>
</body>
</html>
)rawliteral";

