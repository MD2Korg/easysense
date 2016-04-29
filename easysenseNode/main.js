/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var bleno = require('bleno');

// EasySense
// * has status
// * can be sampled
// * provides progress reports
// * can send data
//
var easysense = require('./easysense');

var EasySenseService = require('./easysense-service');

var name = 'EasySense';
var easysenseService = new EasySenseService(new easysense.EasySense());


bleno.on('stateChange', function(state) {
  console.log('on -> stateChange: ' + state);

  if (state === 'poweredOn') {
    bleno.startAdvertising(name, [easysenseService.uuid], function(err) {
        if (err) {
            console.log(err);
        }
    });
  }   
  else {
    if(state === 'unsupported'){
      console.log("NOTE: BLE and Bleno configurations not enabled on board, see README.md for more details...");
    }
    bleno.stopAdvertising();
  }
});

bleno.on('advertisingStart', function(err) {
  console.log('on -> advertisingStart: ' + (err ? 'error ' + err : 'success'));

  if (!err) {
    bleno.setServices([
        easysenseService
    ]);
  }
});

bleno.on('accept', function(clientAddress) {
    console.log("Accepted Connection with Client Address: " + clientAddress);
});

bleno.on('disconnect', function(clientAddress) {
    console.log("Disconnected Connection with Client Address: " + clientAddress);
});