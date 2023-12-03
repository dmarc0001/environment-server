//
// globale variablen für die charts
//
const month_url = '/api/v1/month';
const week_url = '/api/v1/week';
const today_url = '/api/v1/today';
const current_url = '/api/v1/current';
const interval_url = '/api/v1/interval';
const soft_version_url = '/api/v1/version';
const brownout_curent = 2.720;
const warning_current = 2.850;
const min_screen_em = 45;
const axesColors = [
  [ "rgba(255.0,0,0,1.0)", "rgba(255.0,0,0,.2)" ],
  [ "rgba(255.0,255.0,0,1.0)", "rgba(255.0,255.0,0,.2)" ],
  [ "rgba(0.0,120.255,0,.2)", "rgba(60.0,255.0,0,.2)" ],
  [ "rgba(0.0,120.255,0,1.0)", "rgba(0.0,120.255,0,.2)" ]
];

let data_url = today_url;
let data_from_minutes_back = 24 * 60;
let timeInterval = 0;
let t_chart = undefined;
let h_chart = undefined;
let inner_width = undefined;
let data_in_use = false;

//
// options for Charts
//
const t_chart_options = {
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
const t_chart_config = {
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
const h_chart_options = {
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
const h_chart_config = {
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
function init_page() {
  // search main div block, read his style
  let el = document.getElementById('complete');
  let style = window.getComputedStyle(el, null).getPropertyValue('font-size');
  let fontSize = parseFloat(style);
  // window width in em
  inner_width = window.innerWidth / fontSize;
  // add handler for click on buttons
  addEventHandlers();
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
  // get measure intevall from controller
  getIntervalFromController();
  // get version from controller
  setTimeout(
      function() { getSoftwareVersionFromController(); }, 1500);
  // wait for settung interval
  // if interval setting make furter actions
  let waitForInterval = setInterval(
      function() {
        // until timeInterval is not set
        if (timeInterval) {
          clearInterval(waitForInterval);
          // at first time init read values
          getDataFromController();
          getCurrentFromController();
          if (timeInterval == 0) {
            setErrorMessage("Kann intervall nicht vom controller lesen!");
            console.error("data interval timeout!");
          } else {
            setErrorMessage("");
            console.debug("data interval: " + timeInterval);
          }
          // set interval to recive data for chart
          setInterval(
              function() { getDataFromController(); }, 120000);
          setInterval(
              function() { getCurrentFromController(); }, 180000);
        }
      },
      500);
  let vers_elem = document.getElementById('version-div');
  if (vers_elem) {
    vers_elem.hidden = true;
  }
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
    data_from_minutes_back = 1 * 24 * 60;
    data_url = today_url;
    console.debug("set timespan to 24 hours...");
  } else {
    // 24 Stunden, default
    data_from_minutes_back = 3 * 60;
    data_url = today_url;
    console.debug("set timespan to 3 hours...");
  }
  console.debug("set timespan to <" + data_url + ">...");
  getDataFromController();
}

/**** zyclic read acku current from controller ****/
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
          console.debug("controller current value is " + json.current +
                        " V...");
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

/**** read software version from controller ****/
function getSoftwareVersionFromController() {
  let xhr = new XMLHttpRequest();
  console.debug("http request <" + soft_version_url + ">...");
  xhr.open('GET', soft_version_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      let json = JSON.parse(xhr.responseText);
      if (json) {
        if (json.version) {
          console.debug("controller software version is <" + json.version +
                        ">");
          let version_div_elem = document.getElementById("version-div");
          if (version_div_elem) {
            console.debug("set inner html...");
            version_div_elem.innerHTML = "Controller Version: " + json.version;
            version_div_elem.hidden = false;
            setTimeout(
                function() {
                  console.debug("hide version info, timeout...");
                  version_div_elem.hidden = true;
                },
                12000);
          }
        }
      }
    }
  };
  xhr.send();
}

/**** read mesure interval from controller ****/
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
  let hours = "0" + Math.floor(unix_timestamp / (60 * 60));
  // how many secounts over this?
  let r_sec = unix_timestamp % (60 * 60);
  // how many minutes are this?
  let minutes = "0" + Math.floor(r_sec / 60);
  // how many secounds are over
  // let seconds = "0" + r_sec % 60;
  // format this into human readable
  let formattedTime = hours.slice(-2) + ':' + minutes.slice(-2);
  //      hours + ':' + minutes.slice(-2);  // + ':' + seconds.slice(-2);
  return formattedTime;
}

/**** read temp/humidy data from controller ****/
function getDataFromController() {
  setErrorMessage("lade Daten...");
  let yourDate = new Date();
  const offset = yourDate.getTimezoneOffset();
  let xhr = new XMLHttpRequest();
  const xAxis = new Array();
  const t_data_obj = {labels : xAxis, datasets : []};
  const envData = new Object();
  const h_data_obj = {labels : xAxis, datasets : []};
  console.debug("http request <" + data_url + ">...");
  xhr.open('GET', data_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  // reset charts
  h_chart.data = h_data_obj;
  h_chart.update();
  t_chart.data = h_data_obj;
  t_chart.update();
  // prepare to read Data
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      // first add json start and end sign
      // easier as in the controller
      //
      let cEnvData="[\n" + xhr.responseText.substring(1) + "\n]";
      let retValue = prepareJsonData(envData, cEnvData);
      if (retValue) {
        setErrorMessage("");
        //
        // temperature
        //
        let t_timeout = setTimeout(
            function() { makeTemperatureGraph(envData); }, 250);
        //
        // humidy
        //
        let h_timeout = setTimeout(
            function() { makeHumidyGraph(envData); }, 2500);
      } else {
        console.error("axes not load!");
        // TODO: show error describe
        setErrorMessage("Messdaten nicht geladen");
      }
    }
    //
    // END if datatransfer done
    //
  };
  xhr.send();
}

/****  find first valid timestamp in data ****/
function findFirstValidTimestamp(json) {
  let timestamp;
  for (let l_datasetNumber in json) {
    timestamp = parseInt(json[l_datasetNumber].ti);
    if (timestamp) {
      return (timestamp);
    }
  }
  return undefined;
}

/****  find sensors in data ****/
function findSensors(json, envData, id_idx) {
  let id;
  // scan the whole data area first time

  for (let l_datasetNumber in json) {
    //
    let data = json[l_datasetNumber].da;
    for (let sensorNr in data) {
      id = data[sensorNr].id;
      if (envData.ids[id] == undefined) {
        // new id found
        envData.ids[id] = id_idx;
        console.debug("new sensor id <" + id + "> with index <" + id_idx +
                      ">...");
        envData.axes[id_idx] = Array();  // new temp axis for sensor
        id_idx += 1;
      }
    }
  }
  return (id_idx);
}

/**** compute data, recived from controller ****/
function prepareJsonData(envData, rawData) {
  let json = undefined;
  const nowTimeStamp = parseInt(new Date().getTime() / 1000);
  try {
    json = JSON.parse(rawData);
  } catch (error) {
    setErrorMessage("Messdaten fehlerhaft von Controller gesendert!");
    console.error(error);
    return undefined;
  }
  if (data_in_use) {
    console.warn("data preparation already in use. wait....");
    return undefined;
  }
  data_in_use = true;
  // envData object populate
  // overwrite old data, garbabe collector
  // makes clean ;-)
  envData.ids = new Array();
  envData.axes = Array();
  envData.axes[0] = Array();  // x-axis
  envData.axes[1] = Array();  // humidy-axis
  //
  // read the data, find id's
  //
  let id_idx = 2;  // sensors starts by idx 2, id == ids[id]
  let readTimestamp = findFirstValidTimestamp(json);
  if (!readTimestamp) {
    return undefined;
  }
  // which sensors are there, round one dor the data
  id_idx = findSensors(json, envData, id_idx);
  console.debug("sensor id's: " + (id_idx - 2) + " sensors found!");
  //
  // round two, collect data for id's
  //
  // the last timestamp, here as an "virtual" value
  let lastTimeStamp = readTimestamp - timeInterval;
  // offset to readDatasetNumber for gaps in data
  let databaseOffset = 0;
  // from which earlyest point in time i want to read data
  const borderTimeSecounds = (data_from_minutes_back * 60);
  // an now read all the data
  for (let readDatasetNumber in json) {
    // get the dataset number for the axes data
    let axesDatasetNumber = parseInt(readDatasetNumber) + databaseOffset;
    // every timestamped measuring object
    // first timestamp in the file as integer
    readTimestamp = parseInt(json[readDatasetNumber].ti);
    // this was in the past wasMinutes minutes
    let wasForMinutes = formatTimestamp(nowTimeStamp - readTimestamp);
    //
    // is the data before the border?
    //
    if ((nowTimeStamp - readTimestamp) > borderTimeSecounds) {
      // yes? then ignore, offset correct
      // sum with readDataset should be 0 without gaps
      databaseOffset--;
      // start of scale set is this timestamp
      lastTimeStamp = parseInt(json[readDatasetNumber].ti);
      continue;
    }
    //
    // timestamp is in my borders
    // is there an gap in the database?
    // means ist the offset to last dataset too large?
    //
    let awaitedTimestamp = lastTimeStamp + timeInterval;
    let minBorder = awaitedTimestamp - 5;
    let maxBorder = awaitedTimestamp + 5;
    if (readTimestamp < minBorder || readTimestamp > maxBorder) {
      let diff = Math.abs(readTimestamp - awaitedTimestamp);
      console.info("gap in dataset number " + readDatasetNumber +
                   " diff: " + diff + "...");
      let steps = diff / timeInterval;
      if (steps > .9) {
        // okay, the gab have to correct
        readTimestamp = awaitedTimestamp;
        // take an dataset for read the sensors
        let data = json[readDatasetNumber].da;
        steps = Math.round(diff / timeInterval);
        console.debug("this makes " + steps + " steps to fill...");
        // fill in steps zero data datasets
        for (let step = 0; step < steps; step++) {
          console.info("fill in zero dataset step #" + step + " from " + steps +
                       "...");
          wasForMinutes = formatTimestamp(nowTimeStamp - readTimestamp);
          // xAxis is axes[0]
          envData.axes[0][axesDatasetNumber] = wasForMinutes;
          // create a full zero data dataset
          for (let sensorNr in data) {
            let sensor = data[sensorNr];
            // humidy
            if (sensor.id == 0) {
              // humidy sensor
              envData.axes[1][axesDatasetNumber] = -100.0;
            }
            // sensor-id for temperature
            let idx = envData.ids[sensor.id];
            if (idx < 0) {
              console.error("sensor-id not found...");
            } else {
              envData.axes[idx][axesDatasetNumber] = -100.0;
            }
          }
          // move the pointer in the graph axis
          databaseOffset++;
          // mark the last time
          lastTimeStamp = readTimestamp;
          // mark the current time, simulate one reading step
          readTimestamp += timeInterval;
          axesDatasetNumber = parseInt(readDatasetNumber) + databaseOffset;
        }
        // this should the next readet time
        awaitedTimestamp = lastTimeStamp + timeInterval;
      }
    }
    //
    // no gap (more)
    // set to new value
    //
    lastTimeStamp = readTimestamp;
    awaitedTimestamp = readTimestamp + timeInterval;
    // no gap. all right
    // xAxis is axes[0]
    envData.axes[0][axesDatasetNumber] = wasForMinutes;
    //
    // data is set of data for this point in time
    //
    let data = json[readDatasetNumber].da;
    for (let sensorNr in data) {
      let sensor = data[sensorNr];
      //
      // for every sensor value
      //
      // humidy
      if (sensor.id == "HUM") {
        // humidy sensor
        envData.axes[1][axesDatasetNumber] = parseFloat(sensor.hu);
      }
      // get right axis for sensor-id for temperature
      let idx = envData.ids[sensor.id];
      if (idx < 0) {
        console.error("sensor-id not found...");
      } else {
        envData.axes[idx][axesDatasetNumber] = parseFloat(sensor.te);
      }
    }
  }
  data_in_use = false;
  //
  // datasets ready for update
  //
  return (true);
}

/**** paint the temp graphs ****/
function makeTemperatureGraph(envData) {
  // x-axis
  let xAxis = envData.axes[0];
  const t_data_obj = {labels : xAxis, datasets : []};

  // y-axes find....
  for (let yIdx = 2; yIdx < 6; yIdx += 1) {
    if (envData.axes[yIdx] != undefined) {
      console.debug("make data for axis index " + yIdx + "...");
      let c_yAxis = envData.axes[yIdx];
      let c_label = "Sensor " + (yIdx - 2);
      let t_data = {
        label : c_label,
        borderColor : axesColors[yIdx - 2][0],
        backgroundColor : axesColors[yIdx - 2][1],
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
function makeHumidyGraph(envData) {
  // x-axis
  let xAxis = envData.axes[0];
  const h_data_obj = {labels : xAxis, datasets : []};

  // dataset sensor 01
  let h_data00 = {
    label : 'Feuchte Sensor 1',
    backgroundColor : "rgba(0,0,128.0,.6)",
    borderColor : "rgba(0,0,255.0,1.0)",
    data : envData.axes[1],
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
