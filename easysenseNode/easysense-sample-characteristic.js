/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var bleno = require('bleno');
//var easysense = require('./easysense');

function EasySenseSampleCharacteristic(easysense) {
    
    bleno.Characteristic.call(this, {
        uuid: 'ffc1',
        properties: ['write', 'notify'],
        descriptors: [
            new bleno.Descriptor({
                uuid: '2901', //User Description (https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorsHomePage.aspx)
                value: 'Runs EasySenses data collection process and notifies when done'
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
    else if (data.length !== 2) {
        callback(this.RESULT_INVALID_ATTRIBUTE_LENGTH);
    }
    else {
        var command = data.readUInt8(0);  //8-bit int: Accepted range 1-60 seconds
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
        
        
        this.easysense.sample(command);
        callback(this.RESULT_SUCCESS);
    }
};

module.exports = EasySenseSampleCharacteristic;