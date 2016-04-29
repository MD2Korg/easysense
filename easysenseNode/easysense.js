/*jslint node:true,vars:true,bitwise:true,unparam:true */
/*jshint unused:true */

var util = require('util');
var events = require('events');
var spawn = require('child_process').spawn;

function EasySense() {
    events.EventEmitter.call(this);
}

util.inherits(EasySense, events.EventEmitter);

EasySense.prototype.sample = function(seconds) {
    console.log("EasySense: Begin Sampling");
    var self = this;
    var filebase = Date.now();
    var count = 0;
    if (seconds > 60) {
        console.log("Seconds too large: " + seconds);
        self.emit('ready', 69);
    } else if (seconds == 0) {
        console.log("Seconds too small: " + seconds);
        self.emit('ready', 70);
    } else {
    
        console.log('Starting EasySense data collection');
        console.log('/home/root/easysense/radar/easysense.sh' + ' /tmp/' + filebase + ' ' + (seconds*100));
        var radar = spawn('/home/root/easysense/radar/easysense.sh', ['/tmp/' + filebase, seconds*100]);
        
        var interval = setInterval(function() {
            self.emit('progress', count++);
        }, 1000);
        
        radar.on('close', function(code) {
            console.log('child process collectData exited with code ' + code);
            console.log("EasySense measurement complete");
            clearInterval(interval);
            self.emit('ready', 65);    
        });       
    };
};


module.exports.EasySense = EasySense;