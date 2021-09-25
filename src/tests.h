#ifndef TESTS_H
#define TESTS_H
#define CORE_TEST_MODE
extern cs_uint16 Tests_CurrNum;
extern cs_str Tests_Current;
void Tests_NewTask(cs_str name);
#define Tests_Assert(expr, desc) if(!(expr)) { \
	Log_Error("\tFailed: %s.", desc); \
	return false; \
}
cs_bool Tests_PerformAll(void);
#endif
