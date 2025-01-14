--
-- expression evaluation tests that don't fit into a more specific file
--
--
-- Tests for SQLVAlueFunction
--
-- current_date  (always matches because of transactional behaviour)
SELECT date(now())::text = current_date::text;
-- current_time / localtime
SELECT now()::timetz::text = current_time::text;
SELECT now()::timetz(4)::text = current_time(4)::text;
-- current_timestamp / localtimestamp (always matches because of transactional behaviour)
SELECT current_timestamp = NOW();
-- precision
SELECT length(current_timestamp::text) >= length(current_timestamp(0)::text);
select count(*) from date_tbl
  where f1 between '1997-01-01' and '1998-01-01';
select count(*) from date_tbl
  where f1 not between '1997-01-01' and '1998-01-01';
select count(*) from date_tbl
  where f1 between symmetric '1997-01-01' and '1998-01-01';
select count(*) from date_tbl
  where f1 not between symmetric '1997-01-01' and '1998-01-01';
--
-- Test parsing of a no-op cast to a type with unspecified typmod
--
begin;
create table numeric_tbl (f1 numeric(18,3), f2 numeric);
-- bpchar, lacking planner support for its length coercion function,
-- could behave differently
create table bpchar_tbl (f1 character(16) unique, f2 bpchar);
rollback;
--
-- Tests for ScalarArrayOpExpr with a hashfn
--
-- create a stable function so that the tests below are not
-- evaluated using the planner's constant folding.
begin;
rollback;
-- Test with non-strict equality function.
-- We need to create our own type for this.
begin;
create table inttest (a myint);
rollback;
