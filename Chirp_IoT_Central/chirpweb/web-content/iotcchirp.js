var data;
window.onload = function() {

  var audioContext = null;

  /**
   * Instantiates an audio context on which the Chirp will be played.
   */
  try {
    // Fix up for browser prefixing
    window.AudioContext = window.AudioContext || window.webkitAudioContext;
    audioContext = new AudioContext();
  } catch (e) {
    console.log(e);
  }

  // Hello from Chirp - please go to https://developers.chirp.io and get a Chirp Application Key
  window.Chirp = new ChirpConnect('<Chirp application key>', audioContext);
}

function hexToUint8Array(string) {
    var bytes = new Uint8Array(Math.ceil(string.length / 2));
    for (var i = 0; i < bytes.length; i++) bytes[i] = parseInt(string.substr(i * 2, 2), 16);
    return bytes;
}

function hexToAscii(str) {
    hexString = str;
    strOut = '';
    for (x = 0; x < hexString.length; x += 2) {
        strOut += String.fromCharCode(parseInt(hexString.substr(x, 2), 16));
    }
    return strOut;    
}

function sendData() {
    // build the data payload
    // ssid:password:packed_connection_string:magic_number
    // packed_connection_string: appId;deviceId;accessKey
    var ssid = document.configForm.SSID.value;
    var password = document.configForm.PASS.value;
    var connectionString = document.configForm.CONN.value;
    var pin = document.configForm.PIN.value.trim();
    var connectionStringParts = connectionString.split(';');
    var appId = hexToAscii(connectionStringParts[0].split('=')[1].substring(12, 48).replace(/\-/gi, ''));
    var deviceId = connectionStringParts[1].split('=')[1];
    var sharedKey = atob(connectionStringParts[2].substring(16));
    data = ssid + ':' + password + ':' + appId + ';' + deviceId + ';' + sharedKey;

    // XOR encrypt the data with the PIN 
    // Note: this is for example purposes only and should not be considered a secure encryption method
    var encryptedData = "";
    for(var i = 0; i < data.length; i++)
        encryptedData += String.fromCharCode((data[i].charCodeAt() ^ pin[i % 6].charCodeAt()));

    var arr = [...encryptedData].map(encryptedData => encryptedData.charCodeAt());
    var payload = new Uint8Array(arr);

    window.Chirp.send(payload, err => {
        if (err) {
            console.error(err);
        } else {
            console.log('Chirp send was successful');
        }
    });
    return false;
}
