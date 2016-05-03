/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var events = require('events');
var spawn = require('child_process').spawn;

CODES = {
    SUCCESS: 250,
    VERIFIED: 249,
    FAILURE: 248,
    INPUT_TO_LARGE: 247,
    INPUT_TO_SMALL: 246
    //PROGRESS: 0-N: N < 150
};

IDENTIFIERS = {
    EASYSENSE: 'ef1a10d8-272d-4822-b436-39f87205bbac',
    SENSOR: '850a75ab-a811-405d-9817-2867ac36aafc'
}


function EasySense() {
    events.EventEmitter.call(this);
}

util.inherits(EasySense, events.EventEmitter);

EasySense.prototype.sample = function(seconds, filebase) {
    console.log("EasySense: Begin Sampling");
    var self = this;
    var count = 0;
    if (seconds > 60) {
        console.log("Seconds too large: " + seconds);
        self.emit('ready', CODES.INPUT_TO_LARGE);
    } else if (seconds == 0) {
        console.log("Seconds too small: " + seconds);
        self.emit('ready', CODES.INPUT_TO_SMALL);
    } else {
    
        console.log('Starting EasySense data collection');
        console.log('/home/root/easysense/radar/easysense.sh' + ' /home/root/' + filebase + ' ' + (seconds*100));
        var radar = spawn('/home/root/easysense/radar/easysense.sh', ['/home/root/' + filebase, seconds*100]);
        
        var interval = setInterval(function() {
            self.emit('progress', count++);
        }, 1000);
        
        radar.on('close', function(code) {
            console.log('child process collectData exited with code ' + code);
            clearInterval(interval);
            self.emit('ready', CODES.SUCCESS);    
            console.log("EasySense measurement complete");
        });       
    };
};


module.exports.EasySense = EasySense;
module.exports.EasySenseCodes = CODES;
module.exports.EasySenseIdentifiers = IDENTIFIERS;