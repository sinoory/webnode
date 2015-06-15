//本文件主要定义了Iser对象，其属性为各个工具函数
//定义Iser对象
var Iser=
{
	//本页面URL
	url:null,
	//窗口可见区域宽度
	clientWidth:null,
	clientHeight:null,
	//透明层弹窗元素数组
	eleOpacity:[],
	//仿浏览器通知栏类弹窗元素数组
	eleBrowserNotification:[],
	//仿QQ通知类弹窗元素数组
	eleFakeQQ:[],
	//存储元素节点
	eleNode:[],
	/*
		初始化Iser对象
	*/
	
	init:function()
	{
		//初始化属性
		this.url=location.href;
		console.log("初始化URL:"+location.href);
		
		this.clientWidth=document.body.clientWidth;
		console.log("初始化窗口宽："+document.body.clientWidth);
		
		this.clientHeight=document.body.clientHeight;
		console.log("初始化窗口高："+document.body.clientHeight);
		
		var allElements_A=document.body.getElementsByTagName("a");
		//在eleNode中存入对象，对象名为eleLink，属性值为页面所有链接元素节点
		this.eleNode.push({"eleLink":allElements_A});
		
		var allElements=document.body.getElementsByTagName("*");
		//在eleNode中存入对象，对象名为eleAllNode，属性值为页面所有元素节点
		this.eleNode.push({"eleAllNode":allElements});
	},
	/*
		获取指定元素的绝对位置，以JSON返回
		ele：DOM节点对象
	*/
	getOffsetPosition:function(ele)
	{
		if(arguments.length!=1 || ele==null)
		{
			return null;
		}
		var offsetTop=ele.offsetTop;
		var offsetLeft=ele.offsetLeft;
		var offsetWidth=ele.offsetWidth;
		var offsetHeight=ele.offsetHeight;
		//向上循环
		while(ele=ele.offsetParent)
		{
			offsetTop+=ele.offsetTop;
			offsetLeft+=ele.offsetLeft;
		}
		//返回JSON
		return{
		"absTop":parseInt(offsetTop),"absLeft":parseInt(offsetLeft),"absWidth":parseInt(offsetWidth),"absHeight":parseInt(offsetHeight)
		};
	/*
		return{
		"absoluteTop":offsetTop,"absoluteLeft":offsetLeft,"offsetWidth":offsetWidth,"offsetHeight":offsetHeight
		};
	*/
	},
	/*
		去掉元素属性值中的‘px'，转换为整型
		strStyle：属性值
	*/
	clearPx:function(strStyle)
	{
		if(arguments.length!=1)
		{
		 return null;
		}
		var str=String(strStyle);
		return parseInt(str.substring(0,str.length-1));
	},
	/*
		获取元素属性值，从行内式、内嵌式、链接式、导入式获取
		ele:DOM节点
		name:样式名，直接写，不用去掉连接符
		key:标记，数字型为‘n’，背景色为‘b’,字符型为‘s’
	*/
	getCssStyle:function(ele,styleName,key)
	{
		var result;
		//先转换成小写
		styleName=styleName.toLowerCase();
		//获取样式值
		if(styleName && typeof value==='undefined')
		{
			//情况1：行内样式,用style属性获取
			if(ele.styleName && ele.style[styleName])
			{
				result=ele.style[styleName];
			}
			//情况2：嵌入式和外部样式表
			//IE的currentStyle
			else if(ele.currentStyle)
			{
				//替换为驼峰式
				styleName=styleName.replace(/\-([a-z])([a-z]?)/ig,function(s,a,b){
					return a.toUpperCase()+b.toLowerCase();
				});
				result=ele.currentStyle[styleName];
			}
			//W3C方法:Chrome、FireFox、Safari、Opera
			else if(document.defaultView && document.defaultView.getComputedStyle)
			{
				var style=window.getComputedStyle(ele,null);//document.defaultView.getComputedStyle(ele,null);
				result=style.getPropertyValue(styleName);
			}
			//按照类型对得到的数据进行分解
			//数值型
			if(key=='n')
			{
				//去掉px
				if(result.indexOf('px')!=-1)
				{
					result=result.replace(/(px)/i,'');	
				}
				//返回一个浮点型,取小数点两位
				return Math.round((parseFloat(result))*100)/100;
			}
			//颜色型 得到的属性值：rgb(R,G,B)
			else if(key=='b')
			{
				//调用自定义函数，将背景色转换为JSON对象{r,g,b}
				return this.RGBAtoRGB(result);
			}
			else if(key=='s')
			{
				//如果是position等.....
				return result;
			}
		}
	},
	/*
		RGBA转GRB，由于只需要r、g、b三个参数作比较，因此该转化较为简单，除去A阈值即可
	*/
	RGBAtoRGB:function(strRGBA)
	{
		if(arguments.length!=1)
		{
		 return null;
		}
		//先过滤掉空格
		var str=strRGBA.replace(/\s+/g,'');
		//匹配是否为GRBA
		if(str.match(/rgba/i)=='rgba')
		{
			var str=strRGBA.replace(/(rgba)/i,'rgb');//转换RGBA为RGB
			str=str.replace(/,\d{1,}\)/i,')');//去掉A阈值
		}
		//将rgb转换为JSON对象
		str=str.replace(/rgb\(/i,'');
		str=str.replace(/\)/i,'');
		//分割字符串为数组
		var colorArray=str.split(',');
		var r=parseInt(colorArray[0]);
		var g=parseInt(colorArray[1]);
		var b=parseInt(colorArray[2]);
		return{
		//"r":colorArray[0],"g":colorArray[1],"b":colorArray[2]
		"r":r,"g":g,"b":b
		};
	},
	/*
		获取元素有效子节点数量
	*/
	getChildNodeNum:function(ele)
	{
		//变量保存子节点数量
		var result=0;
		//临时变量，存储子节点
		var childArray=new Array();
		var childTemp=new Array();
		var childType=new Array();
		//获取子节点数组
		childArray=ele.childNodes;
		//一种转换
		var child=new Array();
		for(var i=0;i<childArray.length;i++)
		{
			child.push(childArray[i]);
		}
		for(var i=0;i<child.length;i++)
		{
			if(child[i].nodeType==1 && child[i].childNodes.length!=0)
			{
				//遍历当前节点type=1的子节点数量，
				childTemp=child[i].childNodes;
				for (var j=0;j<childTemp.length;j++){
					if(childTemp[j].nodeType==1)
					{
						childType.push(childTemp[j]);
					}
				}
				//如果不为零，将其子节点中type=1的节点数组并入child
				if(childType.length>0){
			//		childArray.concat(childType);
					for(var k=0;k<childType.length;k++)
					{
						child.push(childType[k]);
					}
				}
				//如果为0，result++
				else if(childType.length==0)
				{
					result++
				}
				//清空变量
				childTemp=[];
				childType=[];
			}
		}
		return result;
	},
	/*
		拦截--移除节点或隐藏节点
		type=1:移除节点
		type=2:隐藏节点
	*/
	preventNode:function(ele,type)
	{
		if(arguments.length==2 && ele!=null)
		{
			if(type==1)
			{
				//移除节点	
				ele.parentNode.removeChild(ele);
			}
			else if(type==2)
			{
				//隐藏节点
				ele.style.display="none";
			}
		}
	},
	/*
		获取input的最低z-index值
	*/
	getMinInputZIndex:function(){
		//获取所有input节点
		var inputList=document.body.getElementsByTagName("input");
		var min=this.getCssStyle(inputList[0],'z-index','n');
		for(var i=0;i<inputList.length;i++)
		{
			if(this.getCssStyle(inputList[i],'z-index','n')<min)
			{
				min=this.getCssStyle(inputList[i],'z-index','n');
			}
		}
		return min;
	},
	/*
		检测透明层
	*/
	detectOpacity:function(ele){
		//透明度 && （（高 && 宽）|| （在input上的z-index））
		var theOpacity=this.getCssStyle(ele,'opacity','n');
			
		var theHeight=this.getOffsetPosition(ele).absHeight;
		var theWidth=this.getOffsetPosition(ele).absWidth;
			
		var theAllZIndex=this.getMinInputZIndex();
		var theZIndex=this.getCssStyle(ele,'z-index','n'); 
		if(theOpacity<0.1 && ( (theHeight>=this.clientHeight && theWidth>=this.clientWidth) || (theZIndex>theAllZIndex) ) )
		{
			//如果存在，将透明链接放入eleOpacity数组中
			this.eleOpacity.push(ele);
			console.log("检测出一个透明层弹窗"+ele);
			//删除节点
			this.preventNode(ele,1);
		}
	},
	/*
		检测仿浏览器通知栏
	*/
	detectFakeBrowser:function(ele){
		//(宽度 && 高度) && （位置：top || firstChild） && 子节点数量 && 颜色
		var theHeight=this.getOffsetPosition(ele).absHeight;
		var theWidth=this.getOffsetPosition(ele).absWidth;
		
		var posJudge=this.judgePosition(ele);
		
		var nodeNum=this.getChildNodeNum(ele);
		
		var nodeColor=this.getCssStyle(ele,'background-color','b');
		//筛选条件
		if(	(theWidth>=0.95*this.clientWidth) && (theHeight>0) && (theHeight<=35) && nodeNum<=10  )
		{	
			if( (posJudge==true) 
				|| ((90<=parseInt(nodeColor.r)) && (parseInt(nodeColor.r)<=255) 
					&& (90<=parseInt(nodeColor.g)) && (parseInt(nodeColor.g)<=255) 
					&&(0<=parseInt(nodeColor.b)) &&(parseInt(nodeColor.b)<=90)) 
				)
			{
				
				//将该节点放入数组中
				this.eleBrowserNotification.push(ele);
				console.log("检测仿浏览器通知栏");
				console.log(ele);
				console.log(theWidth);
				console.log(theHeight);console.log(nodeNum);
				console.log(nodeColor);
				console.log(posJudge);
				//隐藏节点
				this.preventNode(ele,2);
				//
			//	ele.setAttribute('test','1');
			}
		}
	},
	/*
		检测仿qq通知
	*/
	detectFakeQQ:function(ele){
		//（right && bottom） && （position=fixed || (width<80 && height<80))
		var theRight=this.getCssStyle(ele,'right','n');
		var theBottom=this.getCssStyle(ele,'bottom','n');
		
		var thePosition=this.getCssStyle(ele,'position','s');
		
		var theWidth=this.getOffsetPosition(ele).absWidth;
		var theHeight=this.getOffsetPosition(ele).absHeight;
		
		if(
			(theRight<=5 && theBottom<=5)&&(thePosition=='fixed' || (theHeight<=80 && theWidth<=80))
		)
		{
			//将该节点放入数组中
		//	this.eleFakeQQ.push(ele);
			console.log("检测仿qq通知");
			console.log(ele);
			//隐藏节点
		//	ele.style.display='none';
		//	console.log(ele.style.display);
			this.preventNode(ele,1);
			console.log('已经隐藏');
		//	ele.setAttribute('test','2');
		}
	},
	/*
		判断通知栏位置
	*/
	judgePosition:function(ele){
		var pos=this.getCssStyle(ele,'position','s');
	//	console.log('position样式：'+pos);
		if(pos!='static')
		{
			if(this.getOffsetPosition(ele).absTop<=5)
			{
				//位置满足条件
			//	console.log("位置满足条件1");
				return true;
			}
		}
		else
		{
			var temp=ele.parentNode.childNodes;
			var obj;
			for(var i=0;i<temp.length;i++)
			{
				//第一个标签节点
				if(temp[i].nodeType==1)
				{
					if(this.isTrueNode(temp[i])==true){
						obj=temp[i];
						break;
					}
				}
			}
			if(obj===ele)
			{
			//	console.log("位置满足条件2");
				return true;
			}
		}
	
		return false;
	},
	/*
		判断一个节点是否为空
	*/
	isTrueNode:function(ele)
	{
		var temp=ele.childNodes;
		for(var i=0;i<temp.length;i++)
		{
			if(temp[i].nodeType==1)
			{
				return true;
			}
		}
		return false;
	},
	/*
		综合研判
	*/
	detectComprehensive:function(){
		//临时变量，存储链接节点
		var tempArrLink=this.eleNode[0].eleLink;
		for(var i=0;i<tempArrLink.length;i++)
		{
			//调用检测透明层
			this.detectOpacity(tempArrLink[i]);
		}
		//临时变量，存储子节点
		var tempArr=this.eleNode[1].eleAllNode;
		for(var i=0;i<tempArr.length;i++)
		{
			//调用检测通知栏
			this.detectFakeBrowser(tempArr[i]);
			//调用检测仿QQ
			this.detectFakeQQ(tempArr[i]);
		}
	}
};
//调用ISER对象
Iser.init();
Iser.detectComprehensive();