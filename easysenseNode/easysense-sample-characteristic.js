/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var bleno = require('bleno');
var easysense = require('./easysense');
var easySenseIdentifiers = require('./easysense').EasySenseIdentifiers;

function EasySenseSampleCharacteristic(easysense) {
    
    bleno.Characteristic.call(this, {
        uuid: easySenseIdentifiers.SENSOR,
        properties: ['write', 'notify'],
        descriptors: [
            new bleno.Descriptor({
                uuid: '2901', //User Description (https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorsHomePage.aspx)
                value: 'EasySenses data collection'
            })
        ]
    });
    
    this.easysense = easysense;
}

util.inherits(EasySenseSampleCharacteristic, bleno.Characteristic);


EasySenseSampleCharacteristic.prototype.onWriteRequest = function(data, offset, withoutResponse, callback) {
    
    if(offset) {
        callback(this.RESULT_ATTR_NOT_LONG);
    }
    else if (data.length !== 5) {
        callback(this.RESULT_INVALID_ATTRIBUTE_LENGTH);
    } 
    else {
        //Example input: 0x3C5728C562 // 60 run on Tue, 03 May 2016 15:36:02 GMT
        var seconds = data.readUInt8(0);  //8-bit int: Accepted range 1-60 seconds
        var filebase = data.readUInt32BE(1).toString(10) + '000'; //32-bit uint: Timestamp epoch
        var self = this;

        this.easysense.once('ready', function(result) {
            if (self.updateValueCallback) {
                var data = new Buffer(1);
                data.writeUInt8(result, 0);
                self.updateValueCallback(data);
                this.removeAllListeners('progress');
            }
        });
        
        this.easysense.on('progress', function(result) {
            console.log("Progress: " + result);
           if (self.updateValueCallback) {
               var data = new Buffer(1);
               data.writeUInt8(result, 0);
               self.updateValueCallback(data,0);
           } 
        });
        
        
        this.easysense.sample(seconds, filebase);
        callback(this.RESULT_SUCCESS);
    }
};

module.exports = EasySenseSampleCharacteristic;