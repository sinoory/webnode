/**
 * Created by zhangtao on 15-4-7.
 * 对json数据进行操作
 */
function Json(){

    /**
     * 转换json数据格式（字符串转为对象，对象转换为字符串）
     * @param {string|object}   json    json格式字符串
     * @param {int}    type     1-将字符串转为对象，2-将对象转为字符串；
     * @return     {obj|string}
     * @api     public
     **/
    this.jsonType=function(json,type){
        if(type==1 && typeof(json)=="string"){
            json=JSON.parse(json);
        }else if(type==2&&typeof(json)=="object"){
            json=JSON.stringify(json);
        }
        return json;
    }

    /**
     * 判断两个json对象数据是否相等；
     * @param {object}   json1    json对象(1维)；
     * @param {objcet}   json2    json对象(1维)；
     * @return     {boolean}
     * @api     public
     *
     **/
    this.isEqual=function(json1,json2){
        if((!json1 || !json2) ||(json1.length!=json2.length)){
            return false;
        }else{
            var r=true;
            for(x in json1){
                if(json1[x]!=json2[x]){
                    r=false;
                }
            }
            return r;
        }
    }

    /**
     * 从json中提取指定值的数据(二维json对象)；
     * @param {object}   json    json对象(1维)；
     * @param {string}   key      键名；
     * @param {string}   val      键值；
     * @return     {object}
     * @api     public
     *
     **/
    this.getJsonByValue=function(json,key,val){
        if(!json || !val ||!key){
            return false;
        }else {
            var r="";
            for(x in json){
                if(json[x][key]==val){
                    r=json[x];
                }
            }
            return r;
        }
    }

    /**
     * 判断对象是否是json数据；
     * @param   {string|object}   json    被判断的数据
     * @return     {bool}
     * @api     public
     *
     **/
    this.isJson=function(data){
        var flag;
        //window.alert(data);
        if(typeof data ==="undefined" || typeof data===null || data==null || typeof data==="number" || typeof data === "boolean"){
            return false;
        }else if(typeof data==="string"){
            //判断第一个和最后一个字符
            var firstChar=data.substr(0, 1);
            var lastChar=data.substr(-1);

            if((firstChar=="{" && lastChar=="}")||(firstChar=="[" && lastChar=="]")) {   //json格式字符串；
                flag = true;
                //console.log(firstChar+":"+lastChar);
                //data=JSON.parse(data);
                /*
                 for (var key in data) {
                 //获取键值,并判断键值是否是json对象?
                 if (typeof data[key] === "object") {
                 //递归判断
                 temp = this.isJson(data[key]);
                 if (!temp)
                 flag = false;
                 } else if(typeof data[key]==="string" || typeof data[key]==="number"){
                 //判断键值是否为undefined,是则将其置为空,
                 if (typeof(data[key]) === "undefined" || data[key] === null) {
                 flag=false;
                 }
                 //alert(key+":"+data[key]+";"+flag);
                 }
                 ;
                 }
                 */
            }else{//普通字符串
                flag=false;
            }

            return flag;
        }
    }

    /**
     * 将json数据字符串格式化输出
     * @param {string|object}   json    json格式字符串或对象
     * @return     {string}
     * @api     public
     **/
    this.format=function(json){
        var reg=null,result='';
        pad=0, PADDING='    ';
        json=jsonType(json,2);
        // 在大括号前后添加换行
        reg = /([\{\}])/g;
        json = json.replace(reg, '\r\n$1\r\n');
        // 中括号前后添加换行
        reg = /([\[\]])/g;
        json = json.replace(reg, '\r\n$1\r\n');
        // 逗号后面添加换行
        reg = /(\,)/g;
        json = json.replace(reg, '$1\r\n');
        // 去除多余的换行
        reg = /(\r\n\r\n)/g;
        json = json.replace(reg, '\r\n');
        // 逗号前面的换行去掉
        reg = /\r\n\,/g;
        json = json.replace(reg, ',');
        //冒号前面缩进
        reg = /\:/g;
        json = json.replace(reg, ': ');
        //对json按照换行进行切分然后处理每一个小块

        /*
         $.each(json.split('\r\n'), function(index, node) {
         var i = 0, indent = 0, padding = '';
         //这里遇到{、[时缩进等级加1，遇到}、]时缩进等级减1，没遇到时缩进等级不变
         if (node.match(/\{$/) || node.match(/\[$/)) {
         indent = 1;
         } else if (node.match(/\}/) || node.match(/\]/)) {
         if (pad !== 0) {
         pad -= 1;
         }
         } else {
         indent = 0;
         }
         //padding保存实际的缩进
         for (i = 0; i < pad; i++) {
         padding += PADDING;
         }
         //添加代码高亮

         result += padding + node;

         pad += indent;
         });
         */
        result=json;
        return result;
    }

    return this;

}
