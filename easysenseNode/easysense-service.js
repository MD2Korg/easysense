/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var bleno = require('bleno');
var easySenseIdentifiers = require('./easysense').EasySenseIdentifiers;

var EasySenseSampleCharacteristic = require('./easysense-sample-characteristic');

function EasySenseService(easysense) {
    bleno.PrimaryService.call(this, {
        uuid: easySenseIdentifiers.EASYSENSE,
        characteristics: [
            new EasySenseSampleCharacteristic(easysense)
        ]
    });
}

util.inherits(EasySenseService, bleno.PrimaryService);

module.exports = EasySenseService;