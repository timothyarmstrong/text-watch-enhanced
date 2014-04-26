function fetchWeather(latitude, longitude) {
  var req = new XMLHttpRequest();
  req.open('GET', 'http://api.openweathermap.org/data/2.1/find/city?' +
      'lat=' + latitude + '&lon=' + longitude + '&cnt=1', true);
  req.onload = function(e) {
    if (req.status == 200) {
      var response = JSON.parse(req.responseText);
      var temperature, icon, city;
      if (response && response.list && response.list.length > 0) {
        var weatherResult = response.list[0];
        temperature = Math.round(weatherResult.main.temp - 273.15);
        city = weatherResult.name;
        Pebble.sendAppMessage({
          'temperature': temperature,
          'city': city
        });
      }
    }
  };
  req.send();
}

function locationSuccess(pos) {
  fetchWeather(pos.coords.latitude, pos.coords.longitude);
}

function locationError(err) {
  Pebble.sendAppMessage({
    'city': 'Location Unavailable',
    'temperature': 'N/A'
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

Pebble.addEventListener("ready", function(e) {
  window.navigator.geolocation.watchPosition(
    locationSuccess, locationError, locationOptions);
});

Pebble.addEventListener("appmessage", function(e) {
    window.navigator.geolocation.getCurrentPosition(
        locationSuccess, locationError, locationOptions);
});
