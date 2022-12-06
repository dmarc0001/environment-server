//
// globale variablen für die charts
// TODO: wenn file-not.found gui ändern
//
let t_chart = undefined;
let h_chart = undefined;
let inner_width = undefined;
let min_em = 45;
let data_url = 'today.json';

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
      min : 12.0,
      max : 35.0
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

function init_page() {
  let el = document.getElementById('complete');
  let style = window.getComputedStyle(el, null).getPropertyValue('font-size');
  let fontSize = parseFloat(style);
  // window width in em
  inner_width = window.innerWidth / fontSize;

  addEventHandlers();

  // set interval to recive data for chart
  setInterval(
      function() { getTempFromController(); }, 90000);
  // at first time init read values
  getTempFromController();

  // create charts
  // temperatur chart
  let t_chart_ctx = document.getElementById('temp_chart').getContext("2d");
  t_chart = new Chart(t_chart_ctx, t_chart_config);
  // humidy chart
  let h_chart_ctx = document.getElementById('humidy_chart').getContext("2d");
  h_chart = new Chart(h_chart_ctx, h_chart_config);
  if (inner_width <= min_em) {
    // value from css @media
    // h_chart.options.scales.y.display = false;
    t_chart.options.plugins.legend.display = false;
    h_chart.options.plugins.legend.display = false;
  } else {
    t_chart.options.plugins.legend.display = true;
    h_chart.options.plugins.legend.display = true;
  }
  t_chart.update();
  h_chart.update();
}

function addEventHandlers() {
  document.getElementById("twenty_four_hours")
      .addEventListener(
          "click", function() { showTimeSpan(0); });
  document.getElementById("seven_days")
      .addEventListener(
          "click", function() { showTimeSpan(1); });
  document.getElementById("thirty_days")
      .addEventListener(
          "click", function() { showTimeSpan(2); });
}

function showTimeSpan(_val) {
  if (_val == 2) {
    // 30 Tage
    data_url = 'month.json';
    console.debug("set timespan to 30 days...");
  } else if (_val == 1) {
    // 7 Tage
    data_url = 'week.json';
    console.debug("set timespan to 7 days...");
  } else {
    // 24 Stunden, defauklt
    data_url = 'today.json';
    console.debug("set timespan to 24 hours...");
  }
  console.debug("set timespan to <" + data_url + ">...");
  getTempFromController();
}

function prepareJsonData(rawData)
{
  let json = JSON.parse(rawData);
  let nowInSeconds = parseInt(new Date().getTime() / 1000);
  let timeInterval = 100000;
  const ids = new Array();
  const axes = Array();
  axes[0] = Array(); // x-axis
  axes[1] = Array(); // humidy-axis
  //
  // read the data, find id's
  //
  let id_idx = 2;   // sensors starts by idx 2, id == ids[id]
  let id;
  let lastTimeStamp = parseInt(json[0].timestamp);
  for( let datasetNumber in json )
  {
    // 
    let data = json[datasetNumber].data;
    for( sensorNr in data )
    {
      id = data[sensorNr].id;
      if (ids[id] == undefined) 
      {
        // new id found
        ids[id] = id_idx;
        console.debug("new sensor id <" + id + "> with index <" + id_idx +
                      ">...");
        axes[id_idx] = Array(); // temp axis for sensor
        id_idx += 1;
      }
    }
    if( datasetNumber > 1 && datasetNumber < 10 )
    {
      //
      // find smallest interval for time
      // this should be the interval of the controller 
      //
      let currentTimeStamp = parseInt(json[datasetNumber].timestamp);
      if( (currentTimeStamp - lastTimeStamp) < timeInterval )
      {
        timeInterval = (currentTimeStamp - lastTimeStamp);
      }
    }
  }
  console.debug("controllers interval is " + timeInterval + " secounds.");
  console.debug("sensor id's: " + (id_idx - 2 )+ " sensors found!");
  //
  // round two, collect data for id's
  //
  // prepare time-gap-control
  lastTimeStamp = parseInt(json[0].timestamp) - timeInterval;
  // offset for gaps
  let databaseOffset = 0;
  for( let datasetNumber in json )
  {
    let currentNumber = parseInt(datasetNumber) + databaseOffset;   
    // every timestamped measuring object
    let timestampVal = json[datasetNumber].timestamp;  // first timestamp in the file
    let timestamp = parseInt(timestampVal); // as integer
    let wasMinutes = formatTimestamp(nowInSeconds - timestamp); // was in the past wasMinutes minutes
    //
    // is ther an gap in the database?
    //
    if( timestamp > (lastTimeStamp + timeInterval) )
    {
      // fill the gap
      console.debug("fill gap in data dataset " +  datasetNumber + "...");
      let curr_stamp = lastTimeStamp + timeInterval;
      while( timestamp > curr_stamp )
      {
        // xAxis is axes[0] 
        wasMinutes = formatTimestamp(nowInSeconds - curr_stamp);
        axes[0][currentNumber] = wasMinutes;
        let data = json[datasetNumber].data;
        for (let sensorNr in data)
        {
          let sensor = data[sensorNr];
          // humidy
          if(sensor.id == 0)
          {
            // humidy sensor
            axes[1][currentNumber] = -100.0;
          }
          // sensor-id for temperature
          let idx = ids[sensor.id];
          if( idx < 0 )
          {
            console.error( "sensor-id not found...");
          }
          else
          {
            axes[idx][currentNumber] = -100.0;
          }
        }
        curr_stamp += timeInterval;
        databaseOffset += 1;
        currentNumber = datasetNumber + databaseOffset;
      }
      console.debug("fill gap in data dataset " +  datasetNumber + " offset now " + databaseOffset + "...");
    }
    // set to new value
    lastTimeStamp = timestamp;
    // no gap. all right
    // xAxis is axes[0] 
    axes[0][currentNumber] = wasMinutes;
    // DEBUG:
    //console.debug("read dataset " + currentNumber + " time diff: " + wasMinutes + "...");
    //
    let data = json[datasetNumber].data;
    for (let sensorNr in data)
    {
      let sensor = data[sensorNr];
      //
      // for every sensor value
      //
      // humidy
      if(sensor.id == 0)
      {
        // humidy sensor
        axes[1][currentNumber] = parseFloat(sensor.humidy);
      }
      // sensor-id for temperature
      let idx = ids[sensor.id];
      if( idx < 0 )
      {
        console.error( "sensor-id not found...");
      }
      else
      {
        axes[idx][currentNumber] = parseFloat(sensor.temp);
      }
      // DEBUG:
      //console.debug("number: " + currentNumber + " time: " + wasMinutes + " sensor: " + sensor.id + " sensor-idx: " + idx + " temp: " + sensor.temp + " hum: " + sensor.humidy + "...");
    } 
  }
  //
  // datasets ready for update
  //
  return(axes);
}

function makeTemperatureGraph(axes)
{
  // x-axis 
  let xAxis = axes[0]; 
  const t_data_obj = {labels : xAxis, datasets : []};

  //
  // colors max 4 axis
  // array [border,background]
  const colors = [
  ["rgba(255.0,0,0,1.0)","rgba(255.0,0,0,.2)"],
  ["rgba(255.0,255.0,0,1.0)","rgba(255.0,255.0,0,.2)"],
  ["rgba(0.0,120.255,0,.2)", "rgba(60.0,255.0,0,.2)"],
  ["rgba(0.0,120.255,0,1.0)","rgba(0.0,120.255,0,.2)"]
  ];

  // y-axes find....
  for(let yIdx = 2; yIdx < 6; yIdx +=1 )
  {
    if(axes[yIdx] != undefined )
    {
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
      if (inner_width <= min_em) {
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
      elem = document.getElementById("stat").innerHTML = "";
    }
  }
}

function makeHumidyGraph(axes)
{
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
  if (inner_width <= min_em) {
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
  elem = document.getElementById("stat").innerHTML = "";
}

function getTempFromController() {
  let yourDate = new Date();
  const offset = yourDate.getTimezoneOffset();

  let xhr = new XMLHttpRequest();
  console.debug("http request <" + data_url + ">...");
  xhr.open('GET', data_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      let axes = prepareJsonData(xhr.responseText);
      //
      // temparature 
      //
      let t_timeout = setTimeout( function() { makeTemperatureGraph(axes);}, 1500 );
      //
      // humidy
      //
      let h_timeout = setTimeout( function() { makeHumidyGraph(axes);}, 4500 );
    } 
    else 
    {
      const xAxis = new Array();
      const t_data_obj = {labels : xAxis, datasets : []};
      const h_data_obj = {labels : xAxis, datasets : []};
      h_chart.data = h_data_obj;
      h_chart.update();
      t_chart.data = h_data_obj;
      t_chart.update();
      // TODO: Fehlernachricht anzeigen
      elem = document.getElementById("stat").innerHTML =
          "Messdaten nicht gefunden";
    }
    //
    // END if datatransfer done
    //
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
  let formattedTime =
      hours + ':' + minutes.slice(-2) + ':' + seconds.slice(-2);
//      hours + ':' + minutes.slice(-2);  // + ':' + seconds.slice(-2);
  return formattedTime;
}
