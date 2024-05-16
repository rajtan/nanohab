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
  var rfm69nodes = [];
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
      else if (gwobj.msgType === "status") {
        var eStatus = document.getElementById('rfm69Status');
        rfm69nodes[gwobj.senderId] = gwobj;
        var aTable = '<table>';
        aTable = aTable.concat(
            '<tr>' +
            '<th>Node</th>' +
            '<th>RSSI</th>' +
            '<th>Packets</th>' +
            '<th>Miss</th>' +
            '<th>Dup</th>' +
            '<th>Last</th>' +
            '</tr>');
        for (var i = 0; i <= 255; i++) {
          if (rfm69nodes[i]) {
            aTable = aTable.concat('<tr>' +
                '<td>' + rfm69nodes[i].senderId + '</td>' +
                '<td>' + rfm69nodes[i].rssi + '</td>' +
                '<td>' + rfm69nodes[i].rxMsgCnt  + '</td>' +
                '<td>' + rfm69nodes[i].rxMsgMiss + '</td>' +
                '<td>' + rfm69nodes[i].rxMsgDup  + '</td>' +
                '<td>' + rfm69nodes[i].message + '</td>' +
                '</tr>');
          }
        }
        aTable = aTable.concat('</table>');
        eStatus.innerHTML = aTable;
      }
      else {
      }
    }
  };
}
</script>
</head>
<body onload="javascript:start();">
<h2>RFM69 Gateway</h2>
<div id="rfm69Config"></div>
<div id="rfm69Status">Waiting for node data</div>
<div id="configureGateway">
  <p><a href="/configGW"><button type="button">Configure Gateway</button></a>
</div>
<div id="FirmwareUpdate">
  <p><a href="/updater"><button type="button">Update Gateway Firmware</button></a>
</div>
</body>
</html>
)rawliteral";

static const char PROGMEM CONFIGUREGW_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
  <title>RFM69 Gateway Configuration</title>
  <style>
    "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
  </style>
</head>
<body>
  <h2>RFM69 Gateway Configuration</h2>
  <a href="/configGWrfm69"><button type="button">RFM69</button></a>
  <p>
  <a href="/configGWmqtt"><button type="button">MQTT</button></a>
  <p>
  <a href="/"><button type="button">Home</button></a>
</body>
</html>
)rawliteral";

static const char PROGMEM CONFIGUREGWRFM69_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
  <title>RFM69 Gateway Configuration</title>
  <style>
    "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
  </style>
</head>
<body>
  <h3>RFM69 Gateway Configuration</h3>
  <form method='POST' action='/configGWrfm69' enctype='multipart/form-data'>
    <label>RFM69 Network ID</label>
    <input type='number' name='networkid' value="%d" min="1" max="255" size="3"><br>
    <label>RFM69 Node ID</label>
    <input type='number' name='nodeid' value="%d" min="1" max="255" size="3"><br>
    <label>RFM69 Encryption Key</label>
    <input type='text' name='encryptkey' value="%s" size="16" maxlength="16"><br>
    <label>RFM69 Power Level</label>
    <input type='number' name='powerlevel' value="%d" min="0" max="31"size="2"><br>
    <label>RFM69 Frequency</label>
    <select name="rfmfrequency">
    <option value="31" %s>315 MHz</option>
    <option value="43" %s>433 MHz</option>
    <option value="86" %s>868 MHz</option>
    <option value="91" %s>915 MHz</option>
    </select><br>
    <label for=hcw>RFM69 HCW</label><br>
    <input type='radio' name='rfm69hcw' id="hcw" value="1" %s> True<br>
    <input type='radio' name='rfm69hcw' id="hcw" value="0" %s> False<br>
    <label>RFM69 AP name</label>
    <input type='text' name='rfmapname' value="%s" size="32" maxlength="32"><br>
    <p><input type='submit' value='Save changes'>
  </form>
  <p><a href="/configGW"><button type="button">Cancel</button></a><a href="/configGWreset"><button type="button">Factory Reset</button></a>
</body>
</html>
)rawliteral";

static const char PROGMEM CONFIGUREGWMQTT_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
  <title>RFM69 Gateway MQTT Configuration</title>
  <style>
    "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
  </style>
</head>
<body>
  <h3>RFM69 Gateway MQTT Configuration</h3>
  <form method='POST' action='/configGWmqtt' enctype='multipart/form-data'>
    <label>MQTT broker</label>
    <input type='text' name='mqttbroker' value="%s" size="32" maxlength="32"><br>
    <label>MQTT client name</label>
    <input type='text' name='mqttclientname' value="%s" size="32" maxlength="32"><br>
    <label>MDNS name</label>
    <input type='text' name='mdnsname' value="%s" size="32" maxlength="32"><br>
    <p><input type='submit' value='Save changes'>
  </form>
  <p><a href="/configGW"><button type="button">Cancel</button></a>
</body>
</html>
)rawliteral";
