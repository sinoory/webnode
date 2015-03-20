function test()
{
   var anquan_jibie = ""; //安全级别
   var tmp = document.getElementById("jg_tips");
   anquan_jibie = tmp.innerHTML;
   console.log("website_check_info" + "#"  + $.trim(anquan_jibie));    
} 
test();
 
