var Clay = require('pebble-clay');
var clayConfig = require('./config.json');

function customFn() {
  var clay = this;
  clay.on('AFTER_BUILD', function() {
    clay.getItemsByType('button').forEach(function(button) {
      button.on('click', function() {
        clay.getAllItems().forEach(function(item) {
          if (item.config.messageKey && item.config.defaultValue !== undefined) {
            item.set(item.config.defaultValue);
          }
        });
      });
    });
  });
}

var clay = new Clay(clayConfig, customFn);
