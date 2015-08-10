var initialized = false;
var configs = {};

Pebble.addEventListener("ready", function() {
  console.log("ready!");
  initialized = true;
  loadLocalConfig();
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  Pebble.openURL('https://dl.dropboxusercontent.com/u/24424405/app/Pebble/Mondaine/1.2/configuration.html?configs=' + encodeURIComponent(JSON.stringify(configs)));
});

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");

  var response = decodeURIComponent(e.response);

  //Using primitive JSON validity and non-empty check
  if (response.charAt(0) == "{" && response.slice(-1) == "}" && response.length > 2) {
    configs = JSON.parse(decodeURIComponent(response));
    console.log("configs = " + JSON.stringify(configs));
    saveLocalConfig(configs);
    returnConfigToPebble();
  } else {
    console.log("Cancelled");
  }
});

function saveLocalConfig(configs) {
  console.log("saveLocalConfig: " + JSON.stringify(configs));
  localStorage.setItem("configs", JSON.stringify(configs));  
  loadLocalConfig();
}

function loadLocalConfig() {
  configs = JSON.parse(localStorage.getItem("configs"));
  if (configs == null) {
    configs = {};
  }
  console.log("loadLocalConfig: " + JSON.stringify(configs));
}

function returnConfigToPebble() {
  console.log("returnConfigToPebble: " + JSON.stringify(configs));
  Pebble.sendAppMessage(configs,
      function(e) {
        console.log("Config info sent to Pebble successfully!");
	      //console.log(e.error.message);
      },
      function(e) {
        console.log("Error sending Config info to Pebble!");
	      console.log(e.error.message);
      }
   );  
}

function dumpObject (obj) {
  var output, property;
  for (property in obj) {
    output += property + ': ' + obj[property] + '; ';
  }
  console.log(output);
}
