// Saved properties of the device that beaconed this application.
var device_id = '';
var device_name = '';

// Known constants for Torch
var uuid_service_sdl =       'b97e65b8-8c23-3193-0a4f-a5064d661def';
var uuid_char_sdl_leds =     'b97e65b9-8c23-3193-0a4f-a5064d661def';
var uuid_char_sdl_whiteled = 'b97e65ba-8c23-3193-0a4f-a5064d661def';

// Interrupt reasons
var TRIPOINT_READ_INT_RANGES = 1
var TRIPOINT_READ_INT_CALIBRATION = 2



var switch_visibility_console_check = "visible";
var switch_visibility_steadyscan_check = "visible";
var steadyscan_on = true;


function update_from_slider (value) {
    app.set_brightness(value);
}


var app = {
    // Application Constructor
    initialize: function () {
        app.log('Initializing application');

        document.addEventListener("deviceready", app.onAppReady, false);
        document.addEventListener("resume", app.onAppReady, false);
        document.addEventListener("pause", app.onAppPause, false);
    },

    // App Ready Event Handler
    onAppReady: function () {
        // Check if this is being opened in Summon
        if (typeof window.gateway != "undefined") {
            app.log("Opened via Summon..");
            // If so, we can query for some parameters specific to this
            // instance.
            device_id = window.gateway.getDeviceId();
            device_name = window.gateway.getDeviceName();

            // Save it to the UI
            document.getElementById('device-id-span').innerHTML = String(device_id);

            ble.isEnabled(app.bleEnabled, app.bleDisabled);

        } else {
            // Not opened in Summon. Don't know what to do with this.
            app.log('Not in summon.');
        }
    },
    // App Paused Event Handler
    onAppPause: function () {
        console.log('DISCONNNNENECCCT');
        ble.disconnect(device_id, app.bleDisconnect, app.bleDisconnectError);
    },

    // Callbacks to make sure that the phone has BLE ENABLED.
    bleEnabled: function () {
        app.log('BLE is enabled.');

        // Need to scan in order to ensure that megster BLE library
        // has the peripheral in its cache
        ble.startScan([], app.bleDeviceFound, app.bleScanError);
    },
    bleDisabled: function () {
        app.log('BLE disabled. Boo.');
    },

    // Callbacks for SCANNING
    bleDeviceFound: function (dev) {
        // Scan found a device, check if its the one we are looking for
        if (dev.id == device_id) {
            // Found our device, stop the scan, and try to connect
            ble.stopScan();

            app.log('Trying to connect to the proper Torch.');
            ble.connect(device_id, app.bleDeviceConnected, app.bleDeviceConnectionError);
        }
    },
    bleScanError: function (err) {
        app.log('Scanning error.');
    },

    // Callbacks for CONNECT
    bleDeviceConnected: function (device) {
        app.log('Successfully connected to Torch');

        // Read initial LED value
        ble.read(device_id, uuid_service_sdl, uuid_char_sdl_whiteled,
           app.bleRawBufferRead, app.bleRawBufferReadError);
    },
    bleDeviceConnectionError: function (err) {
        app.log('Error connecting to Torch: ' + err);

        // Check the error to determine if we should try to connect again,
        // or we need to re-scan for the device.
        if (err == "Peripheral " + device_id + " not found.") {
            // Cannot just reconnect, must rescan
            app.bleEnabled();
        } else {
            app.log('Torch reconnecting try');
            ble.connect(device_id, app.bleDeviceConnected, app.bleDeviceConnectionError);
        }
    },

    // Callbacks for NOTIFY
    // bleRawBufferNotify: function (data) {
    //     // Read to get the rest of the buffer
    //     ble.read(device_id, uuid_service_tritag, uuid_tritag_char_raw,
    //       app.bleRawBufferRead, app.bleRawBufferReadError);
    // },
    // bleRawBufferNotifyError: function (err) {
    //     app.log('Notify raw buffer error.');
    // },

    // Callbacks for READ
    bleRawBufferRead: function (data) {
        app.log('Got starting value: ' + data);
        update_brightness(data);
    },
    bleRawBufferReadError: function (err) {
        app.log('Read raw buf error');
    },

    // Callbacks for DISCONNECT
    bleDisconnect: function () {
        console.log('Successfully disconnected');
        app.log('Disconnected from Torch.');
    },
    bleDisconnectError: function (err) {
        console.log('Error disconnecting.');
        console.log(err);
        app.log('Error disconnecting from Torch');
    },

    bleWriteSuccess: function () {
        app.log('Wrote brightness');
    },
    bleWriteError: function (err) {
        app.log('Write brightness error: ' + err);
    },

    set_brightness: function (dc) {
        var data = new Uint8Array(1);
        data[0] = dc;
        ble.write(device_id, uuid_service_sdl, uuid_char_sdl_whiteled,
            data.buffer, app.bleWriteSuccess, app.bleWriteError);
    },

    update_brightness: function (data) {
        var dv = new DataView(buf, 0);
        var duty_cycle = dv.getUint8(0);

        // Update slider
        var range = document.getElementById("sdl-whiteled-brightness");
        range.value = duty_cycle;
    },

    // Function to Log Text to Screen
    log: function(string) {
    	document.querySelector("#console").innerHTML += (new Date()).toLocaleTimeString() + " : " + string + "<br />";
        document.querySelector("#console").scrollTop = document.querySelector("#console").scrollHeight;
    }
};

app.initialize();
