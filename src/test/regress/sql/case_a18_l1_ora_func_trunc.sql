set grammar to oracle;
set datestyle='ISO,YMD';
set timezone=0;
select trunc(123.67) from dual;
select trunc(2.809,2) from dual;
select trunc(100.987,0) from dual;
select trunc(111.987,-1) from dual;
select trunc(111.987,-3) from dual;
select trunc(exp(2),2) from dual;
select trunc(100.984,'2') from dual;
-----select trunc('100.984','2') from dual;
select trunc(100.984,2.8) from dual;
select trunc(100.984,-2.8) from dual;
select trunc(-100.98467,3) from dual;
select trunc(1000000000000000000,3) from dual;
select trunc(100,50) from dual;
select trunc(sinh(5),3) from dual;
select trunc(3.25,null) from dual;
select trunc(3.25,'') from dual;
select trunc(date'2016-3-15','yyyy') from dual;
select trunc(date'2016-3-15','Mm') from dual;
select trunc(date'2016-3-15','dd') from dual;
select trunc(date'2016-3-15','d') from dual;
select trunc(to_date('2016-3-15 12:25:59','yyyy-mm-dd hh:mi:ss'),'mi') from dual;
select trunc(to_date('2016-3-15 12:25:59','yyyy-mm-dd hh:mi:ss'),'hh24') from dual;
select trunc(date'2016-3-15','yyyy-mm') from dual;
select trunc(to_date('2016-3-15 12:25:59','yyyy-mm-dd hh:mi:ss'),'ss') from dual;
select trunc(to_date('2016-3-15 9:25:59 pm','yyyy-mm-dd hh:mi:ss pm'),'hh24') from dual;
select trunc(to_date('2016-3-15 9:25:59 pm','yyyy-mm-dd hh:mi:ss pm')) from dual;
select trunc(to_date('2016-3-15 9:25:59 pm','yyyy-mm-dd hh:mi:ss pm'),null) from dual;
select trunc(to_date('2016-3-15 9:25:59 pm','yyyy-mm-dd hh:mi:ss pm'),'') from dual;
select trunc(to_date('2016-3-15 12:25:59','yyyy-mm-dd hh:mi:ss') - 1) from dual;
