extern "C" {
#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>
#include <utils/numeric.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_gen_query);
PG_FUNCTION_INFO_V1(pg_gen_and_execute);
PG_FUNCTION_INFO_V1(pg_ai_configure);

Datum pg_gen_query(PG_FUNCTION_ARGS) {
    elog(INFO, "pg_gen_query() called");

    const char* result = "Generated SQL query new";
    PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum pg_gen_and_execute(PG_FUNCTION_ARGS) {
    elog(INFO, "pg_gen_and_execute() called");

    const char* result = "Query generated and executed";
    PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum pg_ai_configure(PG_FUNCTION_ARGS) {
    elog(INFO, "pg_ai_configure() called");

    const char* result = "AI configuration updated";
    PG_RETURN_TEXT_P(cstring_to_text(result));
}

}