/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var bleno = require('bleno');
var easysense = require('./easysense');

function EasySenseCommandCharacteristic(easysense) {
    
    bleno.Characteristic.call(this, {
        uuid: 'ffc0',
        properties: ['read','write', 'notify'],
        descriptors: [
            new bleno.Descriptor({
                uuid: '2901', //User Description (https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorsHomePage.aspx)
                value: 'Gets current or initiates EasySense command'
            }),
             new bleno.Descriptor({
                uuid: '2904',
                value: new Buffer([0x04, 0x01, 0x27, 0xAD, 0x01, 0x00, 0x00 ])
            })
        ]
    });
    
    this.easysense = easysense;
}

util.inherits(EasySenseCommandCharacteristic, bleno.Characteristic);


EasySenseCommandCharacteristic.prototype.onWriteRequest = function(data, offset, withoutResponse, callback) {
    this._value = data;
    console.log("WriteRequest: " + this._value.toString("utf-8"));
    
    if (this._updateValueCallback) {
        console.log('- onWriteRequest: notifying');
        this._updateValueCallback(this._value);
    }
    
    var command = data.readUInt8(0);
    console.log("Command: " + command);
    switch (command) {
        case easysense.EasySenseCommands.RUN_DATA_COLLECTION:
            console.log('RUN_DATA_COLLECTION');
            callback(this.RESULT_SUCCESS);
            break;
        default:
            console.log('DEFAULT');
            callback(this.RESULT_SUCCESS);
            break;
    }

    callback(this.RESULT_SUCCESS);
};

EasySenseCommandCharacteristic.prototype.onReadRequest = function(offset, callback) {
    if (offset) {
        callback(this.RESULT_ATTR_NOT_LONG, null);
    } else {
        var data = new Buffer(2);
        data.writeUInt8(12,0);
        data.writeUInt8(24,1);
        callback(this.RESULT_SUCCESS, data);
    }
};

EasySenseCommandCharacteristic.prototype.onSubscribe = function(maxValueSize, updateValueCallback) {
  console.log('EasySenseCommandCharacteristic - onSubscribe');

  this._updateValueCallback = updateValueCallback;
};

EasySenseCommandCharacteristic.prototype.onUnsubscribe = function() {
  console.log('EasySenseCommandCharacteristic - onUnsubscribe');

  this._updateValueCallback = null;
};

module.exports = EasySenseCommandCharacteristic;