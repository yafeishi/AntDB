set grammar to oracle;
create table t4test(id int, dt timestamp);
insert into t4test values(1,to_date('2000-4-30','YYYY-MM-DD'));
insert into t4test values(2,to_date('1500-1-1 15:23:1','YYYY-MM-DD hh24:mi:ss'));
insert into t4test values(3,to_date('9999-12-30','YYYY-MM-DD'));
insert into t4test values(4,to_date('2001-4-8 BC','YYYY-MM-DD BC'));
insert into t4test values(5,to_date('2100-7-15 22:24:55','YYYY-MM-DD hh24:mi:ss'));
select id, to_char(dt) from t4test order by id;
select id, to_char(dt,'MM-DD-YYYY AD') from t4test order by id;
select id, to_char(dt,'Month D YEAR hh:mi:ss') from t4test order by id;
select id, to_char(dt,'Month DD SYEAR') from t4test order by id;
select id, to_char(dt,'DY') from t4test order by id;
select id, to_char(dt,'DDD') from t4test order by id;
select id, to_char(dt,'Mon D Y hh:mi:ss') from t4test order by id;
select id, to_char(dt,'WW') from t4test order by id;
select id, to_char(dt,'Q') from t4test order by id;
select id, to_char(dt,'SS') from t4test order by id;
select id, to_char(dt,'SSSS') from t4test order by id;
select to_char(24.50,'099.999') from dual;
select to_char(to_char(24.50),'099.999') from dual;
select to_char(to_number('24.50'),'099.999') from dual;
select to_char(24.50,'999,999.999') from dual;
select to_char(892349.5678,'999,999.990') from dual;
select to_char(892349.5678,'$999,999.90') from dual;
select to_char(892349.5678,'$0000,000.909') from dual;
select to_char(892349.5678,'') from dual;
select to_char(-10000,'C99G999D99MI') from dual;
select to_char(-10000.8,'L99G999D99') from dual;
select to_char(-10000.8,'$99G999D99') from dual;
select to_char(-0.898,'B99G999D99') from dual;
select to_char(-9999.898,'L99G999D99EEEE') from dual;
select to_char(9,'RN') from dual;
select to_char(30, 'xxx') from dual;
select to_char(null,'') from dual;
select to_char(to_timestamp('2016-04-12 07:53:56.6259','yyyy-mm-dd hh24:mi:ssxff4'), 'yyyymmdd hh24:mi:ssxff4') FROM dual;
drop table t4test;
