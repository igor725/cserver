#include "core.h"
#include "str.h"
#include "tests.h"

cs_bool Tests_Strings(void) {
	Tests_NewTask("String copying");
	char str1[8] = {0},
	str2[4] = {'l', 'o', 'l'};
	Tests_Assert(String_Copy(str1, 8, str2) == 3, "copy first string to second");
	Tests_Assert(String_Copy(str1, 3, str2) == 2, "copy first string to smaller second array");

	Tests_NewTask("Char finding");
	Tests_Assert(String_Compare(String_FirstChar("_compare_teststring", 'e'), "e_teststring"), "detect first 'e' character");
	Tests_Assert(String_Compare(String_LastChar("_compare_teststring", 'e'), "eststring"), "detect last 'e' character");
	Tests_Assert(String_LastChar("_compare_teststring", 'x') == NULL, "detect invalid character");
	Tests_Assert(String_FindSubstr("afafolafafak", "afafa") != NULL, "detect substring");
	Tests_Assert(String_FindSubstr("afafolafafak", "afafA") == NULL, "detect invalid substring");
	Tests_Assert(String_Compare(String_FindSubstr("afafolaFafak", "aFafa"), "aFafak"), "check detected substring");

	Tests_NewTask("Caseless strings compare");
	Tests_Assert(String_CaselessCompare("_compare_test_string_", "_compare_test_string_"), "caseless compare identical strings");
	Tests_Assert(String_CaselessCompare("_coMpare_tesT_String_", "_ComparE_teSt_sTriNg_"), "caseless compare strings with different cases");
	Tests_Assert(String_CaselessCompare("_XoMpRre_tesT_StNing_", "_CoFparE_teSc_sTriNg_") == false, "caseless compare different strings");
	Tests_Assert(String_CaselessCompare2("_fixed_size_compareFJS*fsdu89", "_fixed_size_compareHJKfhsdHUFHUSDIFuhsdh8yfSD(", 19), "caseless compare fixed strings");

	Tests_NewTask("Cased strings compare");
	Tests_Assert(String_Compare("_compare_test_string__", "_compare_test_string__"), "compare identical strings");
	Tests_Assert(String_Compare("_compare_teSt_string__", "_compAre_test_strinG__") == false, "compare strings with different cases");
	Tests_Assert(String_Compare("_compare_test_strin __", "_compare_test_string__") == false, "compare different strings");
	Tests_Assert(String_Compare("_compare_test_string__", "_compare_test_string_") == false, "compare different sized strings");

	Tests_NewTask("Create allocated copy of string");
	cs_str al = String_AllocCopy("_AlLoCatEdStrInGGGG");
	Tests_Assert(String_Compare(al, "_AlLoCatEdStrInGGGG"), "compare allocated and static strings");

	Tests_NewTask("ASCII to integer");
	Tests_Assert(String_ToInt("500") == 500, "convert ascii decimal to integer");
	Tests_Assert(String_ToInt("x500") == 0, "convert non-decimal to integer");
	Tests_Assert(String_ToInt("test") == 0, "convert regular string to integer");
	Tests_Assert(String_HexToInt("DEAD") == 0xDEAD, "convert hex number to integer");
	Tests_Assert(String_HexToInt("HATE") == 0, "treat regular string as a hex number, then convert to to integer");

	Tests_NewTask("ASCII to float");
	Tests_Assert(String_ToFloat("0.5") == 0.5f, "convert ascii text to float #1");
	Tests_Assert(String_ToFloat("13.37") == 13.37f, "convert ascii text to float #2");
	Tests_Assert(String_ToFloat("0.50000001") == 0.5f, "convert ascii text to float #3");

	Tests_NewTask("Check string length");
	Tests_Assert(String_Length("_length_check_test") == 18, "check string length");
	Tests_Assert(String_Length("") == 0, "check length of empty string");

	Tests_NewTask("Append string to array");
	cs_char *mem = Memory_Alloc(20, 1);
	Tests_Assert(String_Append(mem, 20, "_myteststring") == 13, "append first string to array");
	Tests_Assert(String_Compare(mem, "_myteststring"), "check first appended string");
	Tests_Assert(String_Append(mem, 20, "_add") == 4, "append second string to array");
	Tests_Assert(String_Compare(mem, "_myteststring_add"), "check second appended string");
	Tests_Assert(String_Append(mem, 20, "_ololo") == 2, "append third string to out of space array");
	Tests_Assert(String_Compare(mem, "_myteststring_add_o"), "check third appended string");

	Tests_NewTask("Grow string");
	cs_size newsize = 0;
	Tests_Assert((mem = String_Grow(mem, 4, &newsize)) != NULL, "grow string");
	Tests_Assert(newsize == 24, "check string size after growth");
	Tests_Assert(String_Append(mem, newsize, "lolo") == 4, "append part of third string to growed string");
	Memory_Free(mem);

	Tests_NewTask("Split string by spaces");
	cs_str myteststring = "a1 b2 c3 dDd";
	cs_char argtest[4];
	Tests_Assert(String_Compare(String_FromArgument(myteststring, 0), "a1 b2 c3 dDd"), "check from first element");
	Tests_Assert(String_Compare(String_FromArgument(myteststring, 1), "b2 c3 dDd"), "check from second element");
	Tests_Assert(String_Compare(String_FromArgument(myteststring, 2), "c3 dDd"), "check from third element");
	Tests_Assert(String_Compare(String_FromArgument(myteststring, 3), "dDd"), "check from fourth element");
	Tests_Assert(String_GetArgument(myteststring, argtest, 4, 0) == 2, "get first element");
	Tests_Assert(String_Compare(argtest, "a1"), "check first element");
	Tests_Assert(String_GetArgument(myteststring, argtest, 4, 1) == 2, "get second element");
	Tests_Assert(String_Compare(argtest, "b2"), "check second element");
	Tests_Assert(String_GetArgument(myteststring, argtest, 4, 2) == 2, "get third element");
	Tests_Assert(String_Compare(argtest, "c3"), "check third element");
	Tests_Assert(String_GetArgument(myteststring, argtest, 4, 3) == 3, "get fourth element");
	Tests_Assert(String_Compare(argtest, "dDd"), "check fourth element");

	Tests_NewTask("Base64 encode");
	cs_str mystring = "ololostring";
	cs_size sz = String_Length(mystring),
	b64sz = String_SizeOfB64(sz);
	mem = Memory_Alloc(b64sz, 1);
	Tests_Assert(String_ToB64((const cs_byte *)mystring, sz, mem) == b64sz, "encode string to base64");
	Tests_Assert(String_Compare(mem, "b2xvbG9zdHJpbmc="), "check encoded string");
	Memory_Free(mem);

	Tests_NewTask("Format strings");
	cs_char buf[10];
	Tests_Assert(String_FormatBuf(buf, 10, "%s_%x", "tEst", 1337) == 8, "format string");
	Tests_Assert(String_Compare(buf, "tEst_539"), "check formatted string");
	return true;
}
