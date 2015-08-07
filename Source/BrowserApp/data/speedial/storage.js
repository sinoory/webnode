/**
 * Created by zhangtao on 15-4-7.
 * 基于html5的webstorage进行本地数据存储和操作
 *
 */
( function( $, window, document, undefined ){
    'use strict';


    var Storage=function(){};
    var json=new Json();
    /**
     * 增加或更新本地数据,基于Storage的setItem方法；
     *  @param {string}     key    键名
     *  @param {string|object}     value   需要保持的数据
     *  @param  {boolean}      isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     *  @return   {string|obj}     返回对象或字符串
     *  @api  public;
     *
     * **/
    Storage.prototype.set=function(key,value,isSession){
        var val;
        if(typeof value=="object"){
            val= JSON.stringify(value);
        }else if(typeof value =="string"){
            val=value;
        };
        if(isSession){
            window.sessionStorage(key,val);
        }else{
            window.localStorage.setItem(key,val);
        }

    };

    /**
     *  获取本地数据,基于Storage的getItem方法；
     *  @param  {string}     key    键名
     *  @param  {bool}      isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     *  @return   {string|obj}     返回对象或字符串
     *
     *
     * **/
    Storage.prototype.get=function(key,isSession){

        var value;
        if(isSession){
            value=window.sessionStorage.getItem(k原理ey);
        }else{
            value=window.localStorage.getItem(key);
        }
        if(!value)
            return false;
        //判断是否是json数据格式
        var check=json.isJson(value);
        if(check){  //返回json；
            return JSON.parse(value);
        }else{//字符串格式；
            //判断是否是boolean;
            if(value.toLocaleLowerCase()==="true"){
                return true
            }else if(value.toLocaleLowerCase()==="false"){
                return false;
            }else{
                return value;
            }
        }
    };

    /**
     * 删除制定键名的数据；
     * @param  {string}     key    键名
     * @param  {bool}      isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     * @return   {bool}
     *
     * **/
    Storage.prototype.remove =function (key){
        if(isSession)
            window.sessionStorage.removeItem(key)
        window.localStorage.removeItem(key);
    };

    /**
     * 删除json中指定的某条数据；当前仅支持删除[{},{},{}]或{}格式的json数据；
     * @param  {string}     key    存储键名
     * @param  {object}     position    删除数据的位置     {key:"",name:""}；
     * @param  {bool}      isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     * @param   {function}  callback   回调函数，callback(result):成功返回true,否则返回false;
     * @return   {bool}
     *
     * **/
    Storage.prototype.removeOneItem =function (key,position,isSession,callback){
        var original=this.get(key,isSession);
        if(!original || typeof original!="object"){
            return ;
        }
        if(position){
            if(typeof position.key == "undefined" || position.key=="" || typeof position.value == "undefined" || position.value==""){
                throw "参数错误：请为position指定正确的key和value值！";
            }
        }
        var result=false;
        for(var x in original){
            var it=original;
            if(original.length>0){
                it=original[x];
            }
            if(it[position.key]==position.value){
                original.splice(x,1);
                result=true;
            }
        }
        if(result){
            this.set(key,original,isSession);
        }
        if(typeof callback=="function"){
            callback(result);
        }
        return result;
    };

    /**
     * 向一维或二维json对象添加数据；
     * @param  {string}     key         键名
     * @param  {object}     data        添加的数据,数据格式必须和原数据一致，同为一维或同为二维；
     * @param  {object}     position    要添加的位置；
     * @param  {boolean}    isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     * @param   {function}  callback    回调函数，callback(data,result),data-新数据，result-添加结果，0-未添加，也未排序，1-添加成功，2-只进行排序；
     * @return   {number}   result      0-未添加成功，1-添加成功，2-已存在
     *
     * **/
    Storage.prototype.add =function (key,data,position,isSession,callback){
        //标记结果；
        var result=0;
        var original=this.get(key,isSession);
        if(!original || typeof original!="object"||original.length==0){
            var newData=[];
            newData.push(data);
            this.set(key,newData);
            result=1;
            return result;
        }

        if(original.length>0 && typeof original[0]=="object") { //二维
            //原始长度；
            var len=original.length;
            //是否已经存在；
            var has=false;
            //跟随目标对象的下标，默认最后一个；
            var targetIndex=len-1;
            //被添加数据的下标
            var newIndex=len;
            //只排序
            var onlySetOrder=false;

            if(position){
                if(typeof position.key == "undefined" || position.key=="" || typeof position.value == "undefined" || position.value==""){
                    throw "参数错误：请为position指定正确的key和value值！";
                }
            }
            for(var i=0; i<len; i++){
                if(position){
                    if(original[i][position.key]==position.value){
                        has=true;
                        targetIndex=i;
                    }
                }
                //判断当前数据是否已经存在，如果不存在则添加，存在则重新排序；
                if(json.isEqual(data,original[i])) {
                    has=true;
                    newIndex=i;
                    onlySetOrder=targetIndex+1==newIndex?false:true;
                }
            }
            //在制定位置添加数据
            if(!has || onlySetOrder){
                //重新排序;
                original.splice(targetIndex+1,0,data);
                result=1;
                //删除原数据
                if(onlySetOrder){
                    result=2;
                    if(newIndex<targetIndex+1){
                        original.splice(newIndex,1);
                    }else if(newIndex>targetIndex+1) {
                        original.splice(newIndex+1,1);
                    }
                }
            }
        }else{  //一维，不存在排序问题
            for(var x in data){
                original[x]=data[x];
                result=1;
            }
        }

        if(result>0){
            this.set(key,original,isSession);
        }
        if(typeof callback=="function"){
            callback(data,result);
        }
        return result;

    };


    /**
     * 对json数组重新放置位置；
     * @param {object}  json    json数据对象
     * @param {number}  from    起始位置下标
     * @param {number}  to      放置位置下标
     *
     * */
    Storage.prototype.setPosition=function(json,from,to){
        if(typeof json=="object" && json.lenth>0 && (from >=0 && from<json.length) && (to>=0 && to<json.length) && from!=to){
            var temp=json[from];
            json.splice(to,0,temp);
            //删除原位置数据
            if(from<to){
                json.splice(from,1);
            }else{
                json.splice(from+1,1)
            }
            return json;
        }else{
            return ;
        };
    }

    /**
     * 更新数据；
     * @param  {string}     key         键名
     * @param  {object}     data        新数据；
     * @param  {object}     position    {key:'',value:''}要添加的位置；
     * @param  {boolean}    isSession   是否从session中读取，true-从session 中读取，否则从local中读取
     * @param   {function}  callback    回调函数，callback(data,result),data-新数据，result-添加结果，0-未添加，也未排序，1-添加成功，2-只进行排序；
     * @return   {number}   result      0-更新失败，1-更新成功
     *
     * **/
    Storage.prototype.update=function(key,position,data,isSession,callback){

        var original = this.get(key,isSession);

        if(!original || typeof original!="object"){
            return ;
        }

        if(position){
            if(typeof position.key == "undefined" || position.key=="" || typeof position.value == "undefined" || position.value==""){
                throw "参数错误：请为position指定正确的key和value值！";
            }
        }
        var result=false;
        var has=false;
        if(typeof original=="object"){
            if(original.length>0){//二维；
                for(var x in original){
                    var it=original[x];
                    if(it[position.key]==position.value){
                        has=true;
                        for(var y in data){
                            it[y]=data[y];
                        }
                    }
                }
            }else{  //一维
                for(var y in data) {
                    has=true;
                    original[y] = data[y];
                }
            }
            this.set(key,original,isSession);
            result=true;
        }else if(typeof original=="string"){  //字符串；
            this.set(key,data,isSession);
            has=true;
            result=true;
        }
        if(!has){
            var a=this.add(key,data,position,isSession,callback);
            result=a>0?true:false;
        }else{
            if(typeof callback=="function"){
                callback(result);
            }
        }
        return result;
    };

    /**
     * json数据进行排序；
     * @param {object|array}    data    原数据
     * @param {string}  key     排序键名
     * @param {number}  type    排序类型，1-升序，2-降序
     * @return
     * @api public;
     * */
    Storage.prototype.sort=function(data,key,type){
        if(!data || typeof data!='object'){
            return ;
        }
        if(!key){
            throw new Error("参数错误，请输入排序键名");
        }
        type=!type?1:type;
        if(data.length>0){
            for(var i=0;i<data.length;i++){
                for(var j= i;j<data.length;j++){
                    var it1=data[i];
                    var v1=it1[key];
                    var it2=data[j];
                    var v2=it2[key];
                    if(type==1){    //升序
                        if(v1>v2){
                            var temp=it2;
                            data[j]=it1;
                            data[i]=temp;
                        }
                    }else if(type==2){  //降序
                        if(v1<v2){
                            var temp=it2;
                            data[j]=it1;
                            data[i]=temp;
                        }

                    }
                }
            }
        }
        return data;



    }



    $.storage = function() {
        var S= new Storage();
        return S;
    };

})(jQuery, window, document);
