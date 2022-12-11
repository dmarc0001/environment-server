//
// globale variablen für die charts
// TODO: wenn file-not.found gui ändern
//
const month_url = '/api/v1/month';
const week_url = '/api/v1/week';
const today_url = '/api/v1/today';
const current_url = '/api/v1/current';
const interval_url = '/api/v1/interval';
const brownout_curent = 2.720;
const warning_current = 2.850;
const min_screen_em = 45;
let data_url = today_url;
let data_from_minutes_back = 24 * 60;
let timeInterval = 0;
let t_chart = undefined;
let h_chart = undefined;
let inner_width = undefined;

//
// options for Charts
//
let t_chart_options = {
  responsive : true,
  plugins : {
    title : {
      display : true,
    },
  },
  interaction : {
    intersect : false,
  },
  scales : {
    x : {
      display : true,
      title : {display : true, text : "Differenz zu jetzt, Minuten"}
    },
    y : {
      display : true,
      title : {display : true, text : "Temperatur °C", type : "linear"},
      min : 4.0,
      max : 45.0,
      suggestedMax : 35,
    }
  }
};

//
// t_chart config
//
let t_chart_config = {
  type : "line",
  data : {
    datasets : [ {
      label : 'Sensor 1',
      backgroundColor : "rgba(255.0,0,0,.2)",
      borderColor : "rgba(255.0,0,0,1.0)",
      fill : true,
      cubicInterpolationMode : 'default',
      pointStyle : 'line',
      tension : 0.4
    } ]
  },
  options : t_chart_options
};

//
// humidy chart options
//
let h_chart_options = {
  responsive : true,
  plugins : {
    title : {
      display : true,
    },
  },
  interaction : {
    intersect : false,
  },
  scales : {
    x : {
      display : true,
      title : {display : true, text : "Differenz zu jetzt, Minuten"}
    },
    y : {
      display : true,
      title : {display : true, text : 'Relative Feuchtigkeit'},
      suggestedMin : 0,
      suggestedMax : 100,
      min : 0.0,
    }
  }
};

//
// humidy chart config
//
let h_chart_config = {
  type : "line",
  data : {
    datasets : [ {
      label : 'Sensor 1',
      backgroundColor : "rgba(0,0,128.0,.6)",
      borderColor : "rgba(0,0,255.0,1.0)",
      fill : true,
      cubicInterpolationMode : 'monotone',
      pointStyle : 'circle',
      pointRadius : 2,
      pointHoverRadius : 6,
      tension : 0.8
    } ]
  },
  options : h_chart_options
};

/**** init the page and variables ****/
async function init_page() {
  let el = document.getElementById('complete');
  let style = window.getComputedStyle(el, null).getPropertyValue('font-size');
  let fontSize = parseFloat(style);
  // window width in em
  inner_width = window.innerWidth / fontSize;
  // get intevall from controller
  getIntervalFromController();
  // add handler for click on buttons
  addEventHandlers();
  let counter = 0;
  // while counter done or data_intercal is set
  while (timeInterval == 0 && counter < 50) {
    await new Promise(r => setTimeout(r, 200));
    counter++;
  }
  if (timeInterval == 0) {
    setErrorMessage("Kann intervall nicht vom controller lesen!")
        console.error("data interval timeout!");
  } else {
    setErrorMessage("");
    console.debug("data interval: " + timeInterval);
  }
  // set interval to recive data for chart
  setInterval(
      function() { getTempFromController(); }, 90000);
  setInterval(
      function() { getCurrentFromController(); }, 180000);
  // at first time init read values
  getTempFromController();
  getCurrentFromController();
  // create charts
  // temperatur chart
  let t_chart_ctx = document.getElementById('temp_chart').getContext("2d");
  t_chart = new Chart(t_chart_ctx, t_chart_config);
  // humidy chart
  let h_chart_ctx = document.getElementById('humidy_chart').getContext("2d");
  h_chart = new Chart(h_chart_ctx, h_chart_config);
  // if the screen size to small, make lesser infos
  if (inner_width <= min_screen_em) {
    // value from css @media
    // h_chart.options.scales.y.display = false;
    t_chart.options.plugins.legend.display = false;
    h_chart.options.plugins.legend.display = false;
  } else {
    t_chart.options.plugins.legend.display = true;
    h_chart.options.plugins.legend.display = true;
  }
  // draw then charts with new propertys
  t_chart.update();
  h_chart.update();
}

/**** handler for click on button, native, dirthy, small ****/
function addEventHandlers() {
  document.getElementById("three_hours")
      .addEventListener(
          "click", function() { showTimeSpan(0); });
  document.getElementById("twenty_four_hours")
      .addEventListener(
          "click", function() { showTimeSpan(1); });
  /*
  document.getElementById("seven_days")
      .addEventListener(
          "click", function() { showTimeSpan(2); });
  document.getElementById("thirty_days")
      .addEventListener(
          "click", function() { showTimeSpan(3); });
  */
}

/**** time span to show ****/
function showTimeSpan(_val) {
  if (_val == 3) {
    // 30 Tage
    data_url = month_url;
    data_from_minutes_back = 30 * 24 * 60;
    console.debug("set timespan to 30 days...");
  } else if (_val == 2) {
    // 7 Tage
    data_from_minutes_back = 7 * 24 * 60;
    data_url = week_url;
    console.debug("set timespan to 7 days...");
  } else if (_val == 1) {
    // 1 Tage
    data_from_minutes_back = 7 * 24 * 60;
    data_url = today_url;
    console.debug("set timespan to 24 hours...");
  } else {
    // 24 Stunden, default
    data_from_minutes_back = 3 * 60;
    data_url = today_url;
    console.debug("set timespan to 3 hours...");
  }
  console.debug("set timespan to <" + data_url + ">...");
  getTempFromController();
}

/**** compute data, recived from controller ****/
function prepareJsonData(rawData) {
  let json = undefined;
  let nowInSeconds = undefined;
  try {
    json = JSON.parse(rawData);
    nowInSeconds = parseInt(new Date().getTime() / 1000);
  } catch (error) {
    setErrorMessage("Messdaten fehlerfahft von Controller gesendert!");
    console.error(error);
    return undefined;
  }
  const ids = new Array();
  const axes = Array();
  axes[0] = Array();  // x-axis
  axes[1] = Array();  // humidy-axis
  //
  // read the data, find id's
  //
  let id_idx = 2;  // sensors starts by idx 2, id == ids[id]
  let id;
  let lastTimeStamp = 0;
  if (json[0]) {
    lastTimeStamp = parseInt(json[0].timestamp);
  } else if (json[1]) {
    lastTimeStamp = parseInt(json[1].timestamp);
  }
  for (let datasetNumber in json) {
    //
    let data = json[datasetNumber].data;
    for (sensorNr in data) {
      id = data[sensorNr].id;
      if (ids[id] == undefined) {
        // new id found
        ids[id] = id_idx;
        console.debug("new sensor id <" + id + "> with index <" + id_idx +
                      ">...");
        axes[id_idx] = Array();  // temp axis for sensor
        id_idx += 1;
      }
    }
  }
  console.debug("sensor id's: " + (id_idx - 2) + " sensors found!");
  //
  // round two, collect data for id's
  //
  // prepare time-gap-control
  if (json[0]) {
    lastTimeStamp = parseInt(json[0].timestamp) - timeInterval;
  }
  // offset for gaps
  let databaseOffset = 0;
  const borderTimeSecounds = (data_from_minutes_back * 60);
  for (let datasetNumber in json) {
    let currentNumber = parseInt(datasetNumber) + databaseOffset;
    // every timestamped measuring object
    let timestampVal =
        json[datasetNumber].timestamp;       // first timestamp in the file
    let timestamp = parseInt(timestampVal);  // as integer
    let wasMinutes = formatTimestamp(
        nowInSeconds - timestamp);  // was in the past wasMinutes minutes
    //
    // is the data bevore the border?
    //
    if ((nowInSeconds - timestamp) > borderTimeSecounds) {
      // yes? then ignore, offset correct
      databaseOffset--;
      // start of scale set
      lastTimeStamp = parseInt(json[datasetNumber].timestamp);
      continue;
    }
    //
    // is they an gap in the database?
    //
    if (timestamp > (lastTimeStamp + timeInterval + 20)) {
      // fill the gap
      console.debug("fill gap in data dataset number " + datasetNumber + "...");
      let curr_stamp = lastTimeStamp + timeInterval;
      while (timestamp > curr_stamp) {
        // xAxis is axes[0]
        wasMinutes = formatTimestamp(nowInSeconds - curr_stamp);
        axes[0][currentNumber] = wasMinutes;
        let data = json[datasetNumber].data;
        for (let sensorNr in data) {
          let sensor = data[sensorNr];
          // humidy
          if (sensor.id == 0) {
            // humidy sensor
            axes[1][currentNumber] = -100.0;
          }
          // sensor-id for temperature
          let idx = ids[sensor.id];
          if (idx < 0) {
            console.error("sensor-id not found...");
          } else {
            axes[idx][currentNumber] = -100.0;
          }
        }
        curr_stamp += timeInterval;
        databaseOffset++;
        currentNumber = datasetNumber + databaseOffset;
      }
      console.debug("fill gap in data dataset numbrer " + datasetNumber +
                    " offset now " + databaseOffset + "...");
    }
    // set to new value
    lastTimeStamp = timestamp;
    // no gap. all right
    // xAxis is axes[0]
    axes[0][currentNumber] = wasMinutes;
    // DEBUG:
    // console.debug("read dataset " + currentNumber + " time diff: " +
    // wasMinutes + "...");
    //
    let data = json[datasetNumber].data;
    for (let sensorNr in data) {
      let sensor = data[sensorNr];
      //
      // for every sensor value
      //
      // humidy
      if (sensor.id == 0) {
        // humidy sensor
        axes[1][currentNumber] = parseFloat(sensor.humidy);
      }
      // sensor-id for temperature
      let idx = ids[sensor.id];
      if (idx < 0) {
        console.error("sensor-id not found...");
      } else {
        axes[idx][currentNumber] = parseFloat(sensor.temp);
      }
    }
  }
  //
  // datasets ready for update
  //
  return (axes);
}

/**** paint the temp graphs ****/
function makeTemperatureGraph(axes) {
  // x-axis
  let xAxis = axes[0];
  const t_data_obj = {labels : xAxis, datasets : []};

  //
  // colors max 4 axis
  // array [border,background]
  const colors = [
    [ "rgba(255.0,0,0,1.0)", "rgba(255.0,0,0,.2)" ],
    [ "rgba(255.0,255.0,0,1.0)", "rgba(255.0,255.0,0,.2)" ],
    [ "rgba(0.0,120.255,0,.2)", "rgba(60.0,255.0,0,.2)" ],
    [ "rgba(0.0,120.255,0,1.0)", "rgba(0.0,120.255,0,.2)" ]
  ];

  // y-axes find....
  for (let yIdx = 2; yIdx < 6; yIdx += 1) {
    if (axes[yIdx] != undefined) {
      console.debug("make data for axis index " + yIdx + "...");
      let c_yAxis = axes[yIdx];
      let c_label = "Sensor " + (yIdx - 2);
      let t_data = {
        label : c_label,
        borderColor : colors[yIdx - 2][0],
        backgroundColor : colors[yIdx - 2][1],
        data : c_yAxis,
        fill : true,
        cubicInterpolationMode : 'default',
        pointStyle : 'line',
        tension : 0.4
      };
      t_data_obj.datasets.push(t_data);
      if (inner_width <= min_screen_em) {
        // value from css @media
        t_chart.options.plugins.legend.display = false;
      } else {
        // value from css @media
        // t_chart.options.scales.y.display = true;
        t_chart.options.plugins.legend.display = true;
      }
      t_chart.data = t_data_obj;
      // t_chart.options = t_chart_options;
      t_chart.update();
      // error message delete if exist
      setErrorMessage("");
    }
  }
}

/**** paint the humidy graph ****/
function makeHumidyGraph(axes) {
  // x-axis
  let xAxis = axes[0];
  const h_data_obj = {labels : xAxis, datasets : []};

  // dataset sensor 01
  let h_data00 = {
    label : 'Feuchte Sensor 1',
    backgroundColor : "rgba(0,0,128.0,.6)",
    borderColor : "rgba(0,0,255.0,1.0)",
    data : axes[1],
    fill : true,
    cubicInterpolationMode : 'monotone',
    pointStyle : 'circle',
    pointRadius : 4,
    pointHoverRadius : 6,
    tension : 0.8
  };

  //
  // push data into datasets
  //
  h_data_obj.datasets.push(h_data00);
  if (inner_width <= min_screen_em) {
    // value from css @media
    h_chart.options.plugins.legend.display = false;
  } else {
    // value from css @media
    // h_chart.options.scales.y.display = true;
    h_chart.options.plugins.legend.display = true;
  }

  //
  // assign dataset into chart object
  // and start update
  //
  h_chart.data = h_data_obj;
  h_chart.update();
  setErrorMessage("");
}

/**** initiate reading from contoller and painting graphs ****/
function getTempFromController() {
  setErrorMessage("lade Daten...");
  let yourDate = new Date();
  const offset = yourDate.getTimezoneOffset();
  let xhr = new XMLHttpRequest();
  console.debug("http request <" + data_url + ">...");
  xhr.open('GET', data_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  let axes = undefined;
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      axes = prepareJsonData(xhr.responseText);
      if (axes) {
        setErrorMessage("");
        //
        // temperature
        //
        let t_timeout = setTimeout(
            function() { makeTemperatureGraph(axes); }, 1500);
        //
        // humidy
        //
        let h_timeout = setTimeout(
            function() { makeHumidyGraph(axes); }, 4500);
      } else {
        console.error("axes not load!");
	const xAxis = new Array();
        const t_data_obj = {labels : xAxis, datasets : []};
        const h_data_obj = {labels : xAxis, datasets : []};
        h_chart.data = h_data_obj;
        h_chart.update();
        t_chart.data = h_data_obj;
        t_chart.update();
        // TODO: Fehlernachricht anzeigen
        setErrorMessage("Messdaten nicht geladen");
      }
    }
    //
    // END if datatransfer done
    //
  };
  xhr.send();
}

function getCurrentFromController() {
  let xhr = new XMLHttpRequest();
  console.debug("http request <" + current_url + ">...");
  xhr.open('GET', current_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      let json = JSON.parse(xhr.responseText);
      if (json) {
        if (json.current) {
          console.debug("controller current value is " + json.current + " V...")
              let t_gauge = document.getElementById("temp_gauge");
          if (t_gauge) {
            let l_current = parseFloat(json.current);
            // t_gauge.style.color="rgb(97, 97, 106)";
            t_gauge.style.color = "";
            if (l_current < warning_current) {
              t_gauge.style.color = "rgb(217, 105, 15)";
            }
            if (l_current < brownout_curent) {
              t_gauge.style.color = "rgb(255, 0, 0)";
            }
            t_gauge.innerHTML = "Acku " + json.current + " V";
          }
        }
      }
    }
  };
  xhr.send();
}

function getIntervalFromController() {
  let xhr = new XMLHttpRequest();
  console.debug("http request <" + interval_url + ">...");
  xhr.open('GET', interval_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      try {
        let json = JSON.parse(xhr.responseText);
        if (json) {
          if (json.interval) {
            timeInterval = parseInt(json.interval);
            console.debug("controller interval value is " + timeInterval +
                          " sec...")
          }
        }
      } catch (error) {
        timeInterval = 600;
        console.error(error);
      }
    }
  };
  xhr.send();
}

function varToHex(rgb) {
  let hex = Number(rgb).toString(16);
  if (hex.length < 2) {
    hex = "0" + hex;
  }
  return hex;
}

/**
 * Error message set or undset
 * @param {*} _msg message or ""
 */
function setErrorMessage(_msg) {
  elem = document.getElementById("stat");
  if (elem) {
    elem.innerHTML = _msg;
  }
}

/**** convert timestamps to human readable ****/
function formatTimestamp(unix_timestamp) {
  // how many hours are this?
  let hours = Math.floor(unix_timestamp / (60 * 60));
  // how many secounts over this?
  let r_sec = unix_timestamp % (60 * 60);
  // how many minutes are this?
  let minutes = "0" + Math.floor(r_sec / 60);
  // how many secounds are over
  let seconds = "0" + r_sec % 60;
  // format this into human readable
  let formattedTime = hours + ':' + minutes.slice(-2);
  //      hours + ':' + minutes.slice(-2);  // + ':' + seconds.slice(-2);
  return formattedTime;
}
