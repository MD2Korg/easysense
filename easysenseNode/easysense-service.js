/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var bleno = require('bleno');

//var EasySenseCommandCharacteristic = require('./easysense-command-characteristic');
var EasySenseSampleCharacteristic = require('./easysense-sample-characteristic');

function EasySenseService(easysense) {
    bleno.PrimaryService.call(this, {
        uuid: 'ffe0',
        characteristics: [
//            new EasySenseCommandCharacteristic(easysense),
            new EasySenseSampleCharacteristic(easysense)
        ]
    });
}

util.inherits(EasySenseService, bleno.PrimaryService);

module.exports = EasySenseService;