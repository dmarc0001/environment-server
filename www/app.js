//
// globale variablen für die charts
//
var t_chart = undefined;
var h_chart = undefined;
var inner_width = undefined;
var min_em = 45;
var data_url = 'today.json';

//
// options for Charts
//
var t_chart_options = {
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
var t_chart_config = {
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
var h_chart_options = {
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
      suggestedMax : 100
    }
  }
};

//
// humidy chart config
//
var h_chart_config = {
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
  var el = document.getElementById('complete');
  var style = window.getComputedStyle(el, null).getPropertyValue('font-size');
  var fontSize = parseFloat(style);
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
  var t_chart_ctx = document.getElementById('temp_chart').getContext("2d");
  t_chart = new Chart(t_chart_ctx, t_chart_config);
  // humidy chart
  var h_chart_ctx = document.getElementById('humidy_chart').getContext("2d");
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

function getTempFromController() {
  let yourDate = new Date();
  const offset = yourDate.getTimezoneOffset();

  var xhr = new XMLHttpRequest();
  console.debug("http request <" + data_url + ">...");
  xhr.open('GET', data_url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      //
      // START if datatransfer done
      //
      var json = JSON.parse(xhr.responseText);
      var seconds = parseInt(new Date().getTime() / 1000);
      const xAxis = new Array();
      const yTemp00 = new Array();
      const yTemp01 = new Array();
      const yTemp02 = new Array();
      const yTemp03 = new Array();
      const yHumidy01 = new Array();
      const ids = new Array()
          //
          // round one, id searching
          //
          var id_idx = 0;
      var id;
      for (let i in json) {
        id = json[i].id;
        if (ids[id] == undefined) {
          // new id found
          ids[id] = id_idx;
          console.debug("new sensor id <" + id + "> with index <" + id_idx +
                        ">...");
          id_idx += 1;
        }
      }
      //
      // round two, collect data for id's
      //
      var timestampVal = json[0].timestamp;
      var timestamp = parseInt(timestampVal);
      var wasMinutes = formatTimestamp(seconds - timestamp);
      var datasetNumber = 0;
      xAxis[datasetNumber] = wasMinutes;
      yTemp00[datasetNumber] = -100.0;
      yTemp01[datasetNumber] = -100.0;
      yTemp02[datasetNumber] = -100.0;
      yTemp03[datasetNumber] = -100.0;
      yHumidy01[datasetNumber] = -100.0;
      for (let i in json) {
        id = json[i].id;
        // which id has this sensor?
        id_idx = ids[id];
        if (timestampVal != json[i].timestamp) {
          // new Timestamp
          timestampVal = json[i].timestamp;
          timestamp = parseInt(timestampVal);
          wasMinutes = formatTimestamp(seconds - timestamp);
          datasetNumber += 1;
          xAxis[datasetNumber] = wasMinutes;
          // console.debug("was " + wasMinutes + " in the past");
        }
        // data to which sensorIdx?
        switch (id_idx) {
          case 0:
            yTemp00[datasetNumber] = parseFloat(json[i].temp);
            break;
          case 1:
            yTemp01[datasetNumber] = parseFloat(json[i].temp);
            break;
          case 2:
            yTemp02[datasetNumber] = parseFloat(json[i].temp);
            break;
          case 3:
            yTemp03[datasetNumber] = parseFloat(json[i].temp);
            break;
          case 4:
            yHumidy01[datasetNumber] = parseFloat(json[i].humidy);
            break;
          default:
            break;
        }
      }
      //
      // ready for update
      //
      t_timeout = setTimeout(
          function() {
            // data object for chart
            const t_data_obj = {labels : xAxis, datasets : []};

            // dataset sensor 01
            var t_data00 = {
              label : 'Sensor 1',
              backgroundColor : "rgba(255.0,0,0,.2)",
              borderColor : "rgba(255.0,0,0,1.0)",
              data : yTemp00,
              fill : true,
              cubicInterpolationMode : 'default',
              pointStyle : 'line',
              tension : 0.4
            };

            // dataset sensor 02
            var t_data01 = {
              label : 'Sensor 2',
              backgroundColor : "rgba(255.0,255.0,0,.2)",
              borderColor : "rgba(255.0,255.0,0,1.0)",
              data : yTemp01,
              fill : true,
              cubicInterpolationMode : 'default',
              pointStyle : 'line',
              tension : 0.4
            };

            // dataset sensor 03
            var t_data02 = {
              label : 'Sensor 3',
              backgroundColor : "rgba(60.0,255.0,0,.2)",
              borderColor : "rgba(60.0,255.0,0,1.0)",
              data : yTemp02,
              fill : true,
              cubicInterpolationMode : 'default',
              pointStyle : 'line',
              tension : 0.4
            };

            // dataset sensor 04
            var t_data03 = {
              label : 'Sensor 4',
              backgroundColor : "rgba(0.0,120.255,0,.2)",
              borderColor : "rgba(0.0,120.255,0,1.0)",
              data : yTemp03,
              fill : true,
              cubicInterpolationMode : 'default',
              pointStyle : 'line',
              tension : 0.4
            };

            //
            // push data into datasets
            //
            t_data_obj.datasets.push(t_data00);
            t_data_obj.datasets.push(t_data01);
            t_data_obj.datasets.push(t_data02);
            t_data_obj.datasets.push(t_data03);
            if (inner_width <= min_em) {
              // value from css @media
              // t_chart.options.scales.y.display = false;
              t_chart.options.plugins.legend.display = false;
            } else {
              // value from css @media
              // t_chart.options.scales.y.display = true;
              t_chart.options.plugins.legend.display = true;
            }

            // assign dataset into chart object
            // and start update

            t_chart.data = t_data_obj;
            // t_chart.options = t_chart_options;
            t_chart.update();
          },
          1500);

      //
      // hunidy chart
      //
      // data object for chart
      h_timeout = setTimeout(
          function() {
            const h_data_obj = {labels : xAxis, datasets : []};

            // dataset sensor 01
            var h_data00 = {
              label : 'Feuchte Sensor 1',
              backgroundColor : "rgba(0,0,128.0,.6)",
              borderColor : "rgba(0,0,255.0,1.0)",
              data : yHumidy01,
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
              // h_chart.options.scales.y.display = false;
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
          },
          4500);
    } else {
      const xAxis = new Array();
      const t_data_obj = {labels : xAxis, datasets : []};
      const h_data_obj = {labels : xAxis, datasets : []};
      h_chart.data = h_data_obj;
      h_chart.update();
      t_chart.data = h_data_obj;
      t_chart.update();
    }
    //
    // END if datatransfer done
    //
  };
  xhr.send();
}

function varToHex(rgb) {
  var hex = Number(rgb).toString(16);
  if (hex.length < 2) {
    hex = "0" + hex;
  }
  return hex;
}

function formatTimestamp(unix_timestamp) {
  // how many hours are this?
  var hours = Math.floor(unix_timestamp / (60 * 60));
  // how many secounts over this?
  var r_sec = unix_timestamp % (60 * 60);
  // how many minutes are this?
  var minutes = "0" + Math.floor(r_sec / 60);
  // how many secounds are over
  var seconds = "0" + r_sec % 60;
  // format this into human readable
  var formattedTime =
      hours + ':' + minutes.slice(-2);  // + ':' + seconds.slice(-2);
  return formattedTime;
}
