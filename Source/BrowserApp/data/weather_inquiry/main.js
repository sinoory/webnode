var server = "http://weather.ruyisou.net/";
var weatherPath = server + "weather";
var cityCode = "";
var gInterval = -1;
var cityPath = server + "p";
var noticeTimeDefault = [
	{
		on: true,
		hour: "10时",
		minute: "45分"
	},
	{
		on: true,
		hour: "16时",
		minute: "45分"
	}
]

window.onload = function function_name (argument) {
	// body...
	model.init();
}
var tianqiTable = [
		{
			tianqi: '晴',
			img: 'qing',
			pm_img: 'pm-qing'
		},
		{
			tianqi: '云',
			img: 'duoyun'
		},
		{
			tianqi: '沙',
			img: 'fengsha'
		},
		{
			tianqi: '雷阵雨',
			img: 'leizhenyu'
		},
		{
			tianqi: '雨',
			img: 'yu',
			pm_img: 'pm-yu'
		},
		{
			tianqi: '阴',
			img: 'yin'
		},
		{
			tianqi: '雪',
			img: 'xue'
		},
		{
			tianqi: '霾',
			img: 'wumai'
		}
	]

var cityTable = [
	{
		zn: "北京",
		en: "beijing"
	},
	{
		zn: "上海",
		en: "shanghai"
	},
	{
		zn: "广州",
		en: "guangzhou"
	}
]

var model = {
	weatherData:{},
	city:{},
	cityCode:"",
	cityInfo: null,
	userInfo: null,
	cacheTime: 10*60*1000,
	init: function(){
		this.view = view;
		this.view.init();
		var noticeTime = this.storage.get("notice-time", true);
		if(!noticeTime || noticeTime.length != 2){
			noticeTime = noticeTimeDefault;
			this.storage.set("notice-time", noticeTime, true);
		}
		this.cityInfo = this.storage.get("city-info", true);
		this.userInfo = {
			version: '1.0.0',
			source: 'chrome',
			uuid: '100',
			app: 'plugin'
		};
		if(this.cityInfo){
			this.userInfo.cityCode = this.cityInfo.xian.code;
                        this.getWeatherData(this.userInfo, false);
		}
                else {
			var geolocation = new BMap.Geolocation();
                        var that = this;
			geolocation.getCurrentPosition(function(r){
				if(geolocation.getStatus() == BMAP_STATUS_SUCCESS){
					// 百度地图API功能
					//var point = new BMap.Point(116.331398,39.897445);
					var point = new BMap.Point(r.point.lng,r.point.lat);

					function myFun(result){
						var cityName = result.name;
						//alert("当前定位城市:"+cityName);
                                                if (cityName.indexOf("上海") >= 0)
                                                    that.userInfo.cityCode = "101020100";
                                                else if (cityName.indexOf("北京") >= 0)
                                                    that.userInfo.cityCode = "101010100";
                                                else if (cityName.indexOf("天津") >= 0)
                                                    that.userInfo.cityCode = "101030100";
                                                else if (cityName.indexOf("重庆") >= 0)
                                                    that.userInfo.cityCode = "101040100";

                                                that.getWeatherData(that.userInfo, false);
					}
					var myCity = new BMap.LocalCity();
					myCity.get(myFun);
				}
				else {
					alert('failed'+this.getStatus());
                                        that.getWeatherData(that.userInfo, false);
				}        
			}.bind(that),{enableHighAccuracy: true});
                }
		//this.getWeatherData(this.userInfo, false);
	},
    getWeatherData: function(userInfo, refresh) {
		var lastTime = this.storage.get("lastTime");
		var nowTime = new Date().getTime();
		var weather = this.storage.get("weather", true); 
		if(refresh || !lastTime || nowTime < parseInt(lastTime) || nowTime - parseInt(lastTime) > this.cacheTime || !weather){
			var _this = this;
	        if(!userInfo) userInfo = {};
	        // userInfo.json = true;
	        $.ajax({
	            url: weatherPath,
	            data: userInfo? userInfo:{},
	            cache: false,
	            dataType: "json",
	            // json: "callback",
	            async: true,
	            success: function(obj) {
	        		if(obj && obj.success){
	        			_this.weatherData = obj.data;
	        			_this.weatherData.time = obj.time;
	        			_this.cityCode = obj.data.cityCode;
	        			_this.dataWrap(obj.data);
	        			_this.view.renderMain();
	        			_this.storage.set("lastTime", nowTime);
	        			_this.storage.set("weather", obj, true);
	        			// callback.call(_this.view);//renderMain
	        		}
	            },
	            error: function() {
					var a = arguments[0];
	            }
	        });
		}else{
			console.log("get cache weather");
			this.weatherData = weather.data;
			this.weatherData.time = weather.time;
			this.cityCode = weather.data.cityCode;
			this.dataWrap(weather.data);
			this.view.renderMain();
		}

    },
    getCity: function(cityCode){
    	var _this = this;
    	this.getSheng(cityCode);
    	// this.getShi();
    	// this.getXian();
    },
    getSheng: function(cityCode){
    	var _this = this;
        $.ajax({
            url: cityPath,
            data: {json: true},
            cache: false,
            dataType: "json",
            json: "callback",
            async: true,
            success: function(obj) {
            	_this.city.shengList = obj.data;
            	_this.view.fillSheng();
            	var shiCode = cityCode.substr(3, 4);
            	if(shiCode.substr(0,2) < 6){
            		shiCode = shiCode.substr(0,2)+"01";
            	}
            	_this.getShi(cityCode.substr(3, 2), shiCode);
            },
            error: function() {
				var a = arguments[0];
            }
        });
    },
    getShi: function (pcode, ccode){
    	var _this = this;
        $.ajax({
            url: cityPath,
            data: {json: true,pcode:pcode},
            cache: false,
            dataType: "json",
            json: "callback",
            async: true,
            success: function(obj) {
            	_this.city.shiList = obj.data;
            	_this.view.fillShi();
            	if(!ccode){
            		ccode = obj.data[0].cityCode;
            	}
            	_this.getXian(ccode);
            },
            error: function() {
				var a = arguments[0];
            }
        });
    },
    getXian: function (ccode){
    	var _this = this;
        $.ajax({
            url: cityPath,
            data: {json: true,ccode:ccode},
            cache: false,
            dataType: "json",
            json: "callback",
            async: true,
            success: function(obj) {
            	_this.city.xianList = obj.data;
            	_this.view.fillXian();
            },
            error: function() {
				var a = arguments[0];
            }
        });
    },
    dataWrap: function(weatherData){
    	function tempConvert(temp){
    		var nTemps = temp.replace(/℃/g, "").split("~");
    		var temp1 = parseInt(nTemps[0]);
    		var temp2 = parseInt(nTemps[1]);
    		if(temp1 > temp2){
    			return {highTemp: temp1,lowTemp: temp2};
    		}else{
    			return {highTemp: temp2,lowTemp: temp1};
    		}
    	}
    	var info = weatherData.weatherinfo;
    	var todayDtemps = info.dtemp1 = tempConvert(info.temp1);
    	info.dtemp2 = tempConvert(info.temp2);
    	info.dtemp3 = tempConvert(info.temp3);
    	info.dtemp4 = tempConvert(info.temp4);
    	info.dtemp5 = tempConvert(info.temp5);
    	info.dtemp6 = tempConvert(info.temp6);
		var hours=new Date().getHours();
        var ntempValue= 0,
            dValue=todayDtemps.highTemp - todayDtemps.lowTemp;
        if(hours <= 12){
            ntempValue=dValue/12*hours+todayDtemps.lowTemp;
        }else{
            ntempValue=dValue/12*(24-hours)+todayDtemps.lowTemp;
        };
        info.nowTemp = parseInt(ntempValue);
        var highDiff = 0;
        var lowDiff = 0;
        var tempDiff = 0;
        for(var i = 2; i <= 6; i++){
        	tempDiff = Math.abs(info["dtemp"+i].highTemp - info.dtemp1.highTemp);
        	if(highDiff < tempDiff){
        		highDiff = tempDiff;
        	}
        	tempDiff = Math.abs(info["dtemp"+i].lowTemp - info.dtemp1.lowTemp);
        	if(lowDiff < tempDiff){
        		lowDiff = tempDiff;
        	}
        }
        info.diff = highDiff > lowDiff? highDiff: lowDiff;

        /*pm2.5*/
        var pmData = weatherData.pm25;
        if(pmData){
	        var wPmDate = pmData.pm2_5;
	        var pmColor= "";
	        var pmStr="";
	        if(wPmDate<=35){
	            pmColor="#6bca1e";
	            pmStr="优";
	        }else if(wPmDate>35 && wPmDate<=75){
	            pmColor="#6bca1e";
	            pmStr="良";
	        }else if(wPmDate>75 && wPmDate<=115){
	            pmColor="#d6c617";
	            pmStr="轻度污染";
	        }else if(wPmDate>115 && wPmDate<=150){
	            pmColor="#d6c617";
	            pmStr="中度污染";
	        }else if(wPmDate>150 && wPmDate<=250){
	            pmColor="#c42626";
	            pmStr="重度污染";
	        }else{
	            pmColor="#c42626";
	            pmStr="严重污染";
	        };
	        pmData.qualityInfo = {
	        	pmColor: pmColor,
	        	pmStr: pmStr
	        }
	    }
    },
	storage: {
		db : localStorage,
		get : function(key, isJson) {
			try {
				return isJson === true
						? JSON.parse(this.db.getItem(key))
						: this.db.getItem(key)
			} catch (err) {
				console.log(err);
				return null
			}
		},
		set : function(key, value, isJson) {
			try {
				if (isJson === true) {
					this.db.setItem(key, JSON.stringify(value))
				} else {
					this.db.setItem(key, value)
				}
			} catch (err) {
				console.log(err)
			}
		},
		remove : function(key) {
			try {
				this.db.removeItem(key)
			} catch (err) {
				console.log(err)
			}
		}
	}
}
var view = {
	model: model,
	ref : {
		"main":{
			render:"renderMain"
		},
		"set-wrap":{
			render:"renderSet"
		},
		"trend":{
			render:"renderTrend"
		}
	},
	init: function(){
		this.bindEvents();
		// var noticeTime = this.model.storage.get("notice-time", true);
	},
	bindEvents: function(){
		var $main = $("#main");
		$main.find(".shezhi").bind("click", function(){
			if(!view.model.city.shengList){
				view.model.getCity(view.model.cityCode);
			}
			view.showUp("set-wrap");
		})
		$main.find(".zoushi").bind("click", function(){
			view.showUp("trend");
		})
		$(".back.button").bind("click", function(){
			view.showUp("main");
			$(".tip").hide();
		})
		$(".guanbi").bind("click", function(){
			window.close();
		})
	},
	showUp: function(panelId){
		$("#main, #set-wrap, #trend").hide();
		var panel = $("#"+panelId);

		if(!panel.data("rendered")){
			var renderName = this.ref[panelId].render;
			this[renderName]();
		}
		panel.show();
	},
	renderMain: function(){
		var quality = {

		}
		var weatherData = this.model.weatherData;
		var $main =$("#main");
		$main.data("rendered", true);
		var info = weatherData.weatherinfo;
		var pm25= weatherData.pm25;
		$main.find("#wendu span").text(info.nowTemp + "℃");
		$main.find(".city-name").text(info.city);
		$main.find(".weather").text(info.weather1);
		if(info.nowTemp < -9){
			$main.find("#wendu span").css('font-size', '46px');
		}else{
			$main.find("#wendu span").css('font-size', '60px');
		}
		if(pm25){
			$main.find(".pm25 .from-data").text(pm25.pm2_5);
			$main.find(".wuran .from-data").text(pm25.qualityInfo.pmStr);
			$main.find(".wuran .icon").css("background-color", pm25.qualityInfo.pmColor);
			$main.find(".pm25").show();
			$main.find(".wuran").show();
  	}else{
			$main.find(".pm25").hide();
			$main.find(".wuran").hide();
		}
		$main.find(".date").text(info.date1);
		$main.find(".week").text(info.week);
		function getBgImg(){
			var city_en = '';
			var tianqi_img = '';
			var imgPath = '';
			for(var i =0; i < cityTable.length; i++){
				if(cityTable[i].zn == weatherData.weatherinfo.city.split(" ")[0]){
					city_en = cityTable[i].en;
					break;
				}
			}
			if(!city_en){
				city_en = "qita";
			}
			var date = new Date(weatherData.time);
			var am_pm = date.getHours() < 18 ? 'AM' : 'PM';
			var tianqi = weatherData.weatherinfo.img1['imgTitle'+am_pm];
			for(var i =0; i < tianqiTable.length; i++){
				if(tianqi.indexOf(tianqiTable[i].tianqi) >= 0){
					if(am_pm == 'PM' && tianqiTable[i].pm_img){
						tianqi_img = tianqiTable[i].pm_img;
					}else{
						tianqi_img = tianqiTable[i].img;
					}
					break;
				}
			}
			imgPath = server + 'plugin/images/' + city_en + '/24/' + tianqi_img +'.png';
			return imgPath;
		}
		var bgimg = getBgImg();
		$("body").css("background-image", 'url('+ bgimg +')');
		$(".loading, .percent").hide();
	},
	renderSet: function(citys){
		var $set = $("#set-wrap");
		var storage = this.model.storage;
		var hSelHtml = "";
		var minSelHtml = "";
		var n = "";
		for(var i = 0; i < 24; i++){
			n = i < 10? "0"+i : i;
			hSelHtml+="<option>"+ n +"时</option>"
		}
		n = "";
		for(var i = 0; i < 60; i++){
			n = i < 10? "0"+i : i;
			minSelHtml+="<option>"+ n +"分</option>"
		}
	//	$set.find(".hour-select").append($(hSelHtml));
	//	$set.find(".minute-select").append($(minSelHtml));
	//	var noticeTime = storage.get("notice-time", true);
	/*	if(noticeTime){
			$set.find(".tianqitishi input:eq(0)")[0].checked = noticeTime[0].on;
			$set.find(".hour-select:eq(0)").val(noticeTime[0].hour);
			$set.find(".minute-select:eq(0)").val(noticeTime[0].minute);

			$set.find(".tianqitishi input:eq(1)")[0].checked = noticeTime[1].on;
			$set.find(".hour-select:eq(1)").val(noticeTime[1].hour);
			$set.find(".minute-select:eq(1)").val(noticeTime[1].minute);
		}*/
		var cityInfo = storage.get("city-info", true);
/*		if(cityInfo){
			$set.find(".tianqitishi input:eq(0)")[0].checked = noticeTime[0].on;
			$set.find(".hour-select:eq(0)").val(noticeTime[0].hour);
			$set.find(".minute-select:eq(0)").val(noticeTime[0].minute);

			$set.find(".tianqitishi input:eq(1)")[0].checked = noticeTime[1].on;
			$set.find(".hour-select:eq(1)").val(noticeTime[1].hour);
			$set.find(".minute-select:eq(1)").val(noticeTime[1].minute);
		}*/
		$(".version").text(cif._version);
		function bindEvents(){
			$set.find(".selectCity .button").bind("click",function(){
				var changeFlg = false;
				var citySels = $set.find(".selectCity select");
				var cityInfo = {
					sheng:{
						code: $(citySels[0].children[citySels[0].selectedIndex]).attr("code"),
						name: $(citySels[0].children[citySels[0].selectedIndex]).text()
					},
					shi:{
						code: $(citySels[1].children[citySels[1].selectedIndex]).attr("code"),
						name: $(citySels[1].children[citySels[1].selectedIndex]).text()
					},
					xian:{
						code: $(citySels[2].children[citySels[2].selectedIndex]).attr("code"),
						name: $(citySels[2].children[citySels[2].selectedIndex]).text()
					}
				};
				var _cityInfo = storage.get("city-info", true);
				if(!_cityInfo || (_cityInfo && _cityInfo.xian.code != cityInfo.xian.code)){
					view.model.cityInfo = cityInfo;
					storage.set("city-info", cityInfo, true);
					view.model.userInfo.cityCode = cityInfo.xian.code;
					view.model.getWeatherData(view.model.userInfo, true);
					$("#trend").data("rendered", false);
					var canvas = $("#trend-canvas")[0];
					context = canvas.getContext("2d");
					context.clearRect(0,0,canvas.width,canvas.height);
					changeFlg = true;
				}

			//	var timeSels = $set.find(".tianqitishi select");
			/*	var noticeTime = [
					{
						on: $set.find(".tianqitishi input:eq(0)")[0].checked,
						hour: timeSels.eq(0).val(),
						minute: timeSels.eq(1).val()
					},
					{
						on: $set.find(".tianqitishi input:eq(1)")[0].checked,
						hour: timeSels.eq(2).val(),
						minute: timeSels.eq(3).val()
					}
				]*/
		/*		var t = storage.get("notice-time", true);
				if(noticeTime[0].on != t[0].on || noticeTime[0].hour != t[0].hour || noticeTime[0].minute != t[0].minute ||
					noticeTime[1].on != t[1].on || noticeTime[1].hour != t[1].hour || noticeTime[1].minute != t[1].minute){
					storage.set("notice-time", noticeTime, true);
					changeFlg = true;
				}*/
				if(changeFlg){
					$set.find(".selectCity .tip").show();
					setTimeout(
						function(){
							$set.find(".selectCity .tip").hide();
						}, 1000
					);
				}
			})
		}
		bindEvents();
	},
	fillSheng: function(){
		var model = this.model;
		var cityInfo = model.cityInfo;
		var sheng = $("<select/>").insertBefore($(".selectCity span:eq(1)"));
		var city = this.model.city;
		var list = city.shengList;
		var code = model.weatherData.cityCode.substr(3, 2);
		var name = "";
		var HTMLText = "";
		for(var i = 0, len = list.length; i < len; i++){
			HTMLText+='<option code="'+ list[i].cityCode +'">' + list[i].name +'</option>';
			if(list[i].cityCode == code){
				name = list[i].name;
			}
		}
		sheng.append($(HTMLText));
		sheng.val(name);

		sheng.bind("change", function(){
			$this = $(this);
			$this.nextAll('select').remove();
			var index = this.selectedIndex;
			var code = $this.children().eq(index).attr("code");
			view.model.getShi(code);
		})
	},
	fillShi: function(){
		var model = this.model;
		var cityInfo = model.cityInfo;
		var shi = $("<select/>").insertBefore($(".selectCity span:eq(1)"));
		var city = this.model.city;
		shi.children().remove();
		var HTMLText = "";
		var code = model.weatherData.cityCode.substr(3, 4);
		var name = "";
		for(var i = 0, list = city.shiList, len = list.length; i < len; i++){
			HTMLText+='<option code="'+ list[i].cityCode +'">' + list[i].name +'</option>'
			if(list[i].cityCode == code){
				name = list[i].name;
			}
		}
		shi.append($(HTMLText));
		shi.val(name);

		if(cityInfo){
			shi.val(cityInfo.shi.name);
		}
		shi.bind("change", function(){
			$this = $(this);
			$this.nextAll('select').remove();
			var index = this.selectedIndex;
			var code = $this.children().eq(index).attr("code");
			view.model.getXian(code);
		})
	},
	fillXian: function(){
		var model = this.model;
		var cityInfo = model.cityInfo;
		var xian = $("<select/>").insertBefore($(".selectCity span:eq(1)"));
		var city = this.model.city;
		var code = model.weatherData.cityCode;
		var name = "";
		xian.children().remove();
		var HTMLText = "";
		for(var i = 0, list = city.xianList, len = list.length; i < len; i++){
			HTMLText+='<option code="'+ list[i].cityCode +'">' + list[i].name +'</option>'
			if(list[i].cityCode == code){
				name = list[i].name;
			}
		}
		xian.append($(HTMLText));
		xian.val(name);

		if(cityInfo){
			xian.val(cityInfo.xian.name);
		}
	},
	renderTrend: function(){
		var weatherData = this.model.weatherData;
		var info = weatherData.weatherinfo;
		var $trend =$("#trend");
		$trend.data("rendered", true);
		$trend.find(".cy .text").text(info.index);
		$trend.find(".cx .text").text(info.index_tr);
		$trend.find(".fs .text").text(info.index_uv);
		$trend.find(".city-name").text(info.city);
		var date = new Date(weatherData.time);
		var day = date.getDay();
		var cnNum = ['日','一','二','三','四','五','六'];
		var canvas = $("#trend-canvas")[0];
		$trend.find(".week span").each(function(i, it){
			if(i == 0){
				$(it).text("今天");
			}else{
				$(it).text("周"+cnNum[(day+i)%7]);
			}
		})
		function drawTrend(canvas, lTemperatures, hTemperatures){
			var yBlank = 15;//canvas上下留出的空白
			var height = canvas.height;//canvas高度
			var textXoffset = 14;//温度字体大小
			var r = 4;//圆圈半径
			var lhoffset = 10;//最高温度趋势和最低温度趋势间的空白带
			var tHeight =height - yBlank*2 - textXoffset*2 - lhoffset;

			var hFirstPointY = yBlank+ textXoffset + tHeight/4;
			var lFirstPointY = hFirstPointY + tHeight/2 + lhoffset;
			var hTrend = {
				firstPoint: {x: 15, y:hFirstPointY},
				pointColor: "#ffba00",
				lineColor: "#ffffff",
				color: "#ffffff",
				temperatures: hTemperatures,
				flag:"high"
			}
			var lTrend = {
				firstPoint: {x: 15, y:lFirstPointY},
				pointColor: "#00b4ff",
				lineColor: "#ffffff",
				color: "#ffffff",
				temperatures: lTemperatures,
				flag:"low"
			}

			var yPerDegree = tHeight/4/info.diff;//每一度占的像素数
			var xPerPoint = 56;
			var ctx = canvas.getContext("2d");
			function getPoints(firstPoint, temperatures){
				var points = [];
				for(i = 0; i < temperatures.length; i++){
					var point = {};
					point.x = firstPoint.x + xPerPoint*i;
					point.y = firstPoint.y - yPerDegree*(temperatures[i] - temperatures[0]);
					points.push(point);
				}
				return points;
			}

			function draw(trend){
				var points = getPoints(trend.firstPoint, trend.temperatures);
				var lineColor = trend.lineColor;
				var pointColor = trend.pointColor;
				var textYoffset = trend.flag == "high"? -7:7;
				var textBaseline = trend.flag == "high"? "bottom":"top";
				for(var i = 0, len = points.length; i < len ; i++){
					if(i < len - 1){
						ctx.beginPath();
						ctx.moveTo(points[i].x, points[i].y);
						ctx.lineTo(points[i + 1].x, points[i + 1].y);
						ctx.strokeStyle= lineColor;  
						ctx.lineWidth=2;      
						ctx.stroke();
					}

					ctx.beginPath();
					ctx.arc(points[i].x, points[i].y, r, 0,2*Math.PI);
					ctx.fillStyle = pointColor;
					ctx.fill();
					ctx.strokeStyle = pointColor;
					ctx.stroke();

					ctx.fillStyle    = trend.color;
					ctx.font         = '14px "微软雅黑", "Microsoft YaHei","宋体", Verdana, Geneva, sans-serif';   
					ctx.textBaseline = textBaseline;   
					ctx.fillText  (trend.temperatures[i]+'℃', points[i].x - textXoffset, points[i].y + textYoffset);
				}
			}

			draw(lTrend);
			draw(hTrend);
		}
		var hTemperatures = [];
		var lTemperatures = [];
		for(var i = 1; i < 7; i++){
			hTemperatures.push(info["dtemp"+i].highTemp);
			lTemperatures.push(info["dtemp"+i].lowTemp);
		}
		drawTrend(canvas, lTemperatures, hTemperatures);
	}
}

/*
趋势界面保证差距不管多大都能正确表现
*/
