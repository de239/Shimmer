var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};

function locationSuccess(pos) {
	var weather_url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + owm_api_key;

	xhrRequest(weather_url, 'GET', 
		function(responseText) {
			var json = JSON.parse(responseText);

			if(json) {
				// Temperature in Kelvin
				var temperature = Math.round(json.main.temp - 273.15);

				var dict = {
					'KEY_TEMPERATURE' : temperature
				};

				Pebble.sendAppMessage(dict);
			} else {
				console.log("null response from OpenWeatherMap");
			}
		}
	);
}

function locationError(err) {
	console.log('Error requesting location!');
}

function getWeather() {
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000}
	);
}

Pebble.addEventListener('ready', 
	function(e) {
		Pebble.sendAppMessage({});

		getWeather();
	}
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
	function(e) {
		getWeather();
	}                     
);
