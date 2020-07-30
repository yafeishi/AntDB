set grammar to oracle;
--select rpad('~！@#￥%er', 20) from dual;
--select rpad('你好', 5) from dual;
select rpad('cd  ae',5) from dual;
select rpad(to_char(45), 5) from dual;
select rpad(3.545, 5) from dual;
select rpad(4, 5) from dual;
select rpad('a','4') from dual;
select rpad('a','4.84') from dual;
select rpad('abcdegf',3) from dual;
--select rpad('你好',3) from dual;
select rpad('abcdegf',tan(1)) from dual;
--select rpad('abcdegf',100*100) from dual;
select rpad('abc',-2) from dual;
select rpad('abc',5,'*') from dual;
select rpad('abc',5,'123456') from dual;
select rpad('abc',5,' ') from dual;
select rpad('abc',5,6) from dual;
select rpad('abc',5,1.6) from dual;
select rpad('abc',5,to_char(2)) from dual;
select rpad('abc',5,to_number(2)) from dual;
select rpad('abc',5,3*2) from dual;
select rpad('abc',5,tan(2)) from dual;
--select rpad('abc',5,'你好') from dual;
select rpad('',null) from dual;
select rpad(null,null,'') from dual;


CREATE TABLE t4test (id int,txt varchar);
insert into t4test values(1,rpad('12',8));
insert into t4test values(2,rpad('12',8,'*'));
select * from t4test where txt=rpad('12',8,'*') order by id;
update t4test set txt=rpad(txt,10,'@');
select * from t4test order by id;
drop table t4test;