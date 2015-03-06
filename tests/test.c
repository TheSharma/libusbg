#include <usbg/usbg.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>

#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

#include "usbg-test.h"

/**
 * @file tests/test.c
 */

#define USBG_TEST(name, test, setup, teardown) \
	{"setup "#test, setup, UNIT_TEST_FUNCTION_TYPE_SETUP}, \
	{name, test, UNIT_TEST_FUNCTION_TYPE_TEST}, \
	{"teardown "#test, teardown, UNIT_TEST_FUNCTION_TYPE_TEARDOWN}

/**
 * @brief Simplest udcs names
 * @details Used to go through init when testing other things
 */
static char *simple_udcs[] = {
	"UDC1",
	"UDC2",
	NULL
};

/**
 * @brief Simplest functions names
 * @details Used to go through init when testing other things
 */
static struct test_function simple_funcs[] = {
	{
		.type = F_ECM,
		.instance = "0"
	}, {
		.type = F_ACM,
		.instance = "0"
	},

	TEST_FUNCTION_LIST_END
};

/**
 * @brief All functions types
 * @details When testing with this in state, check if all func types are
 * processed correctly
 */
static struct test_function all_funcs[] = {
	{
		.type = F_SERIAL,
		.instance = "serial_instance0"
	}, {
		.type = F_ACM,
		.instance = "acm_instance0"
	}, {
		.type = F_OBEX,
		.instance = "obex_instance0"
	}, {
		.type = F_ECM,
		.instance = "ecm_instance0"
	}, {
		.type = F_SUBSET,
		.instance = "subset_instance0"
	}, {
		.type = F_NCM,
		.instance = "ncm_instance0"
	}, {
		.type = F_EEM,
		.instance = "eem_instance0"
	}, {
		.type = F_RNDIS,
		.instance = "rndis_instance0"
	}, {
		.type = F_PHONET,
		.instance = "phonet_instance0"
	}, {
		.type = F_FFS,
		.instance = "ffs_instance0"
	},

	TEST_FUNCTION_LIST_END
};

static struct test_function same_type_funcs[] = {
	{
		.type = F_SERIAL,
		.instance = "0"
	}, {
		.type = F_SERIAL,
		.instance = "1"
	}, {
		.type = F_SERIAL,
		.instance = "2"
	},

	TEST_FUNCTION_LIST_END
};

/**
 * @brief No functions at all
 * @details Check if gadget with no functions (or config with no bindings)
 * is processed correctly.
 */
static struct test_function no_funcs[] = {
	TEST_FUNCTION_LIST_END
};

/**
 * @brief Simple configs
 * @details Used to pass through init when testing other things
 */
static struct test_config simple_confs[] = {
	{
		.label = "c",
		.id = 1,
		.bindings = simple_funcs
	},

	TEST_CONFIG_LIST_END
};

/**
 * @brief Configs bound to all avaible function types
 */
static struct test_config all_bindings_confs[] = {
	{
		.label = "c",
		.id = 2,
		.bindings = no_funcs
	}, {
		.label = "c",
		.id = 4,
		.bindings = all_funcs
	},

	TEST_CONFIG_LIST_END
};

/**
 * @brief Simplest gadget
 */
static struct test_gadget simple_gadgets[] = {
	{
		.name = "g1",
		.udc = "UDC1",
		.configs = simple_confs,
		.functions = simple_funcs
	},

	TEST_GADGET_LIST_END
};

/**
 * @brief Gadgets with all avaible functions
 */
static struct test_gadget all_funcs_gadgets[] = {
	{
		.name = "all_funcs_gadget1",
		.udc = "UDC1",
		.configs = all_bindings_confs,
		.functions = all_funcs
	},

	TEST_GADGET_LIST_END
};

/**
 * @brief Simple state
 */
static struct test_state simple_state = {
	.configfs_path = "config",
	.gadgets = simple_gadgets,
	.udcs = simple_udcs
};

/**
 * @brief State with all functions avaible
 */
static struct test_state all_funcs_state = {
	.configfs_path = "all_funcs_configfs",
	.gadgets = all_funcs_gadgets,
        .udcs = simple_udcs
};

/**
 * @brief Prepare test_state with one gadget containing given function list
 * @details For testing only functions. We put them in a gadget as simply
 * as possible.
 * @param[in] func Pointer to list of functions
 * @return Pointer to test state with given functions
 */
static struct test_state *put_func_in_state(struct test_function *func)
{
	struct test_state *st;
	struct test_gadget *g;
	struct test_config *c;
	char **udcs;

	st = malloc(sizeof(*st));
	if (st == NULL)
		fail();
	free_later(st);

	/* Do not need config */
	c = calloc(1, sizeof(*c));
	if (c == NULL)
		fail();
	free_later(c);

	g = calloc(2, sizeof(*g));
	if (g == NULL)
		fail();
	free_later(g);

	udcs = calloc(2, sizeof(*udcs));
	if (udcs == NULL)
		fail();
	free_later(udcs);

	g[0].name = "g1";
	g[0].udc = "UDC1";
	g[0].configs = c;
	g[0].functions = func;

	udcs[0] = "UDC1";

	st->configfs_path = "config";
	st->gadgets = g;
	st->udcs = udcs;

	prepare_state(st);

	return st;
}

/**
 * @brief Setup simple state with some gadgets, configs and functions
 */
static void setup_simple_state(void **state)
{
	prepare_state(&simple_state);
	*state = &simple_state;
}

/**
 * @brief Setup state with all avaible functions
 */
static void setup_all_funcs_state(void **state)
{
	prepare_state(&all_funcs_state);
	*state = &all_funcs_state;
}

/**
 * @brief Setup state with few functions of the same type
 */
static void setup_same_type_funcs_state(void **state)
{
	*state = put_func_in_state(same_type_funcs);
}

/**
 * @brief Tests usbg_get_gadget function with given state
 * @details Check if gadgets are returned correctly
 */
static void test_get_gadget(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);
		assert_gadget_equal(g, &st->gadgets[i]);
	}
}

/**
 * @brief Tests usbg_get_gadget with non-existing gadget name
 * @details Check if get_gadget will not find non-existing gadgets and
 * will not cause crash.
 */
static void test_get_gadget_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	g = usbg_get_gadget(s, "non-existing-gadget");
	assert_null(g);
}

/**
 * @brief Tests usbg_get_first_gadget function
 * @details Check if gadget returned by get_first_gadget is actually first one
 */
static void test_get_first_gadget(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	g = usbg_get_first_gadget(s);
	assert_non_null(g);
	assert_gadget_equal(g, &st->gadgets[0]);
}

/**
 * @brief Tests get_first_gadget with invalid arguments
 */
static void test_get_first_gadget_fail(void **state)
{
	usbg_gadget *g;

	g = usbg_get_first_gadget(NULL);
	assert_null(g);
}

/**
 * @brief Tests getting name of gadget
 * @details Check if gadget name is returned correctly
 */
static void test_get_gadget_name(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	const char *name;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);
		name = usbg_get_gadget_name(g);
		assert_string_equal(name, st->gadgets[i].name);
	}
}

/**
 * @brief Tests getting name length of gadget
 * @details Check if returned name length is equal original
 */
static void test_get_gadget_name_len(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	int len;
	int expected;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		expected = strlen(st->gadgets[i].name);
		len = usbg_get_gadget_name_len(g);
		assert_int_equal(len, expected);
	}
}

/**
 * @brief Tests getting name of gadget with invalid arguments
 * @details Check if trying to get name of wrong (non-existing) gadget
 * will not cause crash, but return NULL as expected.
 */
static void test_get_gadget_name_fail(void **state)
{
	const char *name;

	name = usbg_get_gadget_name(NULL);
	assert_null(name);
}

/**
 * @brief Tests copying gadget's name
 * @details Check if copying gadget name copy actual name correctly
 */
static void test_cpy_gadget_name(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		ret = usbg_cpy_gadget_name(g, name, USBG_MAX_NAME_LENGTH);
		assert_int_equal(ret, USBG_SUCCESS);
		assert_string_equal(name, st->gadgets[i].name);
	}
}

/**
 * @brief Test copying gadet name with invalid arguments
 * @details Check if trying to copy gadget name into non-existing buffer,
 * or give invalid buffer length, or invalid gadget will be handled by library
 * and return correct error codes
 */
static void test_cpy_gadget_name_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		ret = usbg_cpy_gadget_name(g, name, 0);
		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);

		ret = usbg_cpy_gadget_name(g, NULL, USBG_MAX_NAME_LENGTH);
		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);
	}

	ret = usbg_cpy_gadget_name(NULL, name, USBG_MAX_NAME_LENGTH);
	assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);
}

/**
 * @brief Tests init by comparing test state and usbg state
 * @details Check if usbg state after init match given state and
 * if init returned success code
 */
static void test_init(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	assert_state_equal(s, st);
}

/**
 * @brief Test getting function by name
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 */
static void test_get_function(void **state)
{
	for_each_test_function(state, assert_func_equal);
}

/**
 * @brief Tests usbg_get_function with some non-existing functions
 * @details Check if get function will return NULL, when invalid
 * functions names and types are passed as arguments and will not cause crash.
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 */
static void test_get_function_fail(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_function *f = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	g = usbg_get_first_gadget(s);
	assert_non_null(g);

	f = usbg_get_function(g, F_ACM, "non-existing-instance");
	assert_null(f);

	f = usbg_get_function(g, 9001, "0");
	assert_null(f);
}


/**
 * @brief Tests function type translation to string
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 * @details Check if get_function_type_str returns proper strings for all types.
 */
static void test_get_function_type_str(void **state)
{
	struct {
		usbg_function_type type;
		const char *str;
	} types[] = {
		{F_SERIAL, "gser"},
		{F_ACM, "acm"},
		{F_OBEX, "obex"},
		{F_ECM, "ecm"},
		{F_SUBSET, "geth"},
		{F_NCM, "ncm"},
		{F_EEM, "eem"},
		{F_RNDIS, "rndis"},
		{F_PHONET, "phonet"},
		{F_FFS, "ffs"}
	};

	const char *str;
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		str = usbg_get_function_type_str(types[i].type);
		assert_non_null(str);
		assert_string_equal(str, types[i].str);
	}
}

/**
 * @brief Tests function type translation to string with unknown funcs
 * @param[in] state Not used parameter
 * @details Check if get_function_type_str returns NULL, when given
 * function type is unknown.
 */
static void test_get_function_type_str_fail(void **state)
{
	const char *str;

	str = usbg_get_function_type_str(-1);
	assert_null(str);
}

/**
 * @brief Get instance of given function and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_get_function_instance(usbg_function *f, struct test_function *tf)
{
	const char *str;

	str = usbg_get_function_instance(f);
	assert_string_equal(str, tf->instance);
}

/**
 * @brief Tests getting function instance from usbg_function structure
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 * @details Check if returned instance name is correct.
 */
static void test_get_function_instance(void **state)
{
	for_each_test_function(state, try_get_function_instance);
}

/**
 * @brief Cpy instance of given usbg function and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_cpy_function_instance(usbg_function *f, struct test_function *tf)
{
	char str[USBG_MAX_NAME_LENGTH];
	int ret;
	int small_len = 2;

	ret = usbg_cpy_function_instance(f, str, USBG_MAX_NAME_LENGTH);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_string_equal(str, tf->instance);

	ret = usbg_cpy_function_instance(f, str, small_len);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_memory_equal(str, tf->instance, small_len - 1);
	assert_int_equal(str[small_len - 1], '\0');
}

/**
 * @brief Tests copying function instance from usbg_function structure into buffer
 * @param[in] state Pointer to pointer to correctly initialized state
 * @details Check if buffer contains expected string
 */
static void test_cpy_function_instance(void **state)
{
	for_each_test_function(state, try_cpy_function_instance);
}

/**
 * @brief Get function type and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f by type
 */
static void try_get_function_type(usbg_function *f, struct test_function *tf)
{
	usbg_function_type type;

	type = usbg_get_function_type(f);
	assert_int_equal(type, tf->type);
}

/**
 * @brief Tests getting function type
 * @details Check if getting function type returns what was expected.
 * State must be proper (init must end with success).
 * @param[in] state Pointer to pointer to correctly initialized state
 */
static void test_get_function_type(void **state)
{
	for_each_test_function(state, try_get_function_type);
}

/**
 * @brief Check if function instance length is correct
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_get_function_instance_len(usbg_function *f, struct test_function *tf)
{
	size_t len;
	len = usbg_get_function_instance_len(f);
	assert_int_equal(len, strlen(tf->instance));
}

/**
 * @brief Tests getting length of function instance name
 * @details Check if returned instance name length matches
 * actual length of instance name
 * @param[in] state Pointer to pointer to correctly initialized state
 */
static void test_get_function_instance_len(void **state)
{
	for_each_test_function(state, try_get_function_instance_len);
}

/**
 * @brief Tests getting configfs path from usbg state
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_configfs_path(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	const char *path;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	path = usbg_get_configfs_path(s);
	assert_path_equal(path, st->configfs_path);
}

/**
 * @brief Tests getting configfs path length from usbg state
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_configfs_path_len(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	int ret, len;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	ret = usbg_get_configfs_path_len(s);
	len = strlen(st->configfs_path);
	assert_int_equal(ret, len);
}

/**
 * @brief Tests copying configfs path into buffer
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_cpy_configfs_path(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	char path[PATH_MAX];
	int ret;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	ret = usbg_cpy_configfs_path(s, path, PATH_MAX);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_path_equal(path, st->configfs_path);
}

/**
 * @brief Tests getting config by name
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config(void **state)
{
	for_each_test_config(state, assert_config_equal);
}

static void test_get_config_without_label(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_config *c = NULL;
	struct test_state *ts;
	struct test_gadget *tg;
	struct test_config *tc;

	ts = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(ts, &s);
	*state = s;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);
		for (tc = tg->configs; tc->label; tc++) {
			c = usbg_get_config(g, tc->id, NULL);
			assert_non_null(c);
			assert_config_equal(c, tc);
		}
	}
}

/**
 * @bried Tests getting non-existing config
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_fail(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_config *c = NULL;
	struct test_state *ts;
	struct test_gadget *tg;

	ts = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(ts, &s);
	*state = s;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);

		c = usbg_get_config(g, 0, "non-existing-config");
		assert_null(c);

		c = usbg_get_config(g, -9001, "c");
		assert_null(c);

		c = usbg_get_config(g, -9001, NULL);
		assert_null(c);
	}
}

/**
 * @brief Get config label and check it
 * @param[in] c Usbg config
 * @param[in] tc Test config which should match c
 */
static void try_get_config_label(usbg_config *c, struct test_config *tc)
{
	const char *label;
	label = usbg_get_config_label(c);
	assert_string_equal(label, tc->label);
}

/**
 * @brief Tests getting config label
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_label(void **state)
{
	for_each_test_config(state, try_get_config_label);
}

/**
 * @brief Check config id with test structure
 * @param[in] c Usbg config
 * @param[in] tc Test config which should match c
 */
static void try_get_config_id(usbg_config *c, struct test_config *tc)
{
	int id;
	id = usbg_get_config_id(c);
	assert_int_equal(id, tc->id);
}

/**
 * @brief Tests getting config id
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_id(void **state)
{
	for_each_test_config(state, try_get_config_id);
}

/**
 * @brief cleanup usbg state
 */
static void teardown_state(void **state)
{
	usbg_state *s = NULL;

	s = (usbg_state *)(*state);
	if (s != NULL)
		usbg_cleanup(s);

	cleanup_stack();
}

/* Custom macro for defining test with given name and fixed teardown function */
#define USBG_TEST_TS(name, test, setup) \
	USBG_TEST(name, test, setup, teardown_state)

/**
 * @page usbg_tests Tests
 * @brief This is list of test cases
 * @tests_start
 */

#ifndef DOXYGEN
static UnitTest tests[] = {
#endif

	/**
	 * @usbg_test
	 * @test_desc{test_init_simple,
	 * Check if init was successfull on simple configfs state,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_simple",
		     test_init, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_init_all_funcs,
	 * Check if init was successfull when all avaible functions
	 * are present in configfs,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_all_funcs",
		     test_init, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_simple,
	 * Check if simple gadget will be correcty returned,
	 * usbg_get_gadget}
	 */
	USBG_TEST_TS("test_get_gadget_simple",
		     test_get_gadget, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_fail_simple,
	 * Check if getting non-existing and wrong gadgets cause
	 * expected failure and error codes are correct,
	 * usbg_get_gadget}
	 */
	USBG_TEST_TS("test_get_gadget_fail_simple",
		     test_get_gadget_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_first_gadget_simple,
	 * Check if gadget returned by get_first_gadget
	 * is actually first one,
	 * usbg_get_first_gadget}
	 */
	USBG_TEST_TS("test_get_first_gadget_simple",
		     test_get_first_gadget, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_first_gadget_fail,
	 * Check if getting first gadget from state returns NULL when
	 * invalid parameters are passed,
	 * usbg_get_first_gadget}
	 */
	unit_test(test_get_first_gadget_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_simple,
	 * Check if returned gadget name matches expected value,
	 * usbg_get_gadget_name}
	 */
	USBG_TEST_TS("test_get_gadget_name_simple",
		     test_get_gadget_name, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_len,
	 * Check if returned simple gadget name length matches expected value,
	 * usbg_get_gadget_name}
	 */
	USBG_TEST_TS("test_get_gadget_name_len_simple",
		     test_get_gadget_name_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_fail,
	 * Check if trying to get name of invalid gadget
	 * cause expected failure (name is null),
	 * usbg_get_gadget_name}
	 */
	unit_test(test_get_gadget_name_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_gadget_name_simple,
	 * Check if getting simple gadget name into buffer work as expected,
	 * usbg_cpy_gadget_name}
	 */
	USBG_TEST_TS("test_cpy_gadget_name_simple",
		     test_cpy_gadget_name, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_gadget_name_fail_simple,
	 * Check if writting gadget name into buffer fail when
	 * invalid parameters are passed,
	 * usbg_cpy_gadget_name}
	 */
	USBG_TEST_TS("test_cpy_gadget_name_fail_simple",
		     test_cpy_gadget_name_fail, setup_simple_state),
	/**
	* @usbg_test
	 * @test_desc{test_get_function_simple,
	 * Check if function can be correctly get from simple state,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_simple",
		     test_get_function, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_all_funcs,
	 * Check if getting function work on all function types,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_all_funcs",
		     test_get_function, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_same_type_funcs,
	 * Check if having multiple functions with the same type does not
	 * cause failure
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_same_type_funcs",
		     test_get_function, setup_same_type_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_fail_simple,
	 * Check if trying to get invalid function's name ends
	 * with expected error,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_fail_simple",
		     test_get_function_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_instance_simple,
	 * Check if getting simple instance returns what expected,
	 * usbg_get_function_instance}
	 */
	USBG_TEST_TS("test_get_function_instance_simple",
		     test_get_function_instance, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_function_instance_simple,
	 * Check if copying simple instance into buffer returns what expected,
	 * usbg_cpy_function_instance}
	 */
	USBG_TEST_TS("test_cpy_function_instance_simple",
		     test_cpy_function_instance, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_simple,
	 * Check if function type is returned correctly,
	 * usbg_get_function_type}
	 */
	USBG_TEST_TS("test_get_function_type_simple",
		     test_get_function_type, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_all_funcs,
	 * Check if all function types are returned correctly,
	 * usbg_get_function_type}
	 */
	USBG_TEST_TS("test_get_function_type_all_funcs",
		     test_get_function_type, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_instance_len_simple,
	 * Check if function instance length is returned correctly,
	 * usbg_get_function_instance_len}
	 */
	USBG_TEST_TS("test_get_function_instance_len_simple",
		     test_get_function_instance_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_str,
	 * Compare returned function types strings with expected values,
	 * usbg_get_function_type_str}
	 */
	unit_test(test_get_function_type_str),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_str_fail,
	 * Try to get type string of unknown type,
	 * usbg_get_function_type_str}
	 */
	unit_test(test_get_function_type_str_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_get_configfs_path_simple,
	 * heck if simple configfs path was returned correctly,
	 * usbg_get_configfs_path}
	 */
	USBG_TEST_TS("test_get_configfs_path_simple",
		     test_get_configfs_path, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_configfs_path_len,
	 * Check if configfs path length is correctly calculated,
	 * usbg_get_configfs_path_len}
	 */
	USBG_TEST_TS("test_get_configfs_path_len_simple",
		     test_get_configfs_path_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_configfs_path_simple,
	 * Copy simple configfs path into buffer and compare with original,
	 * usbg_cpy_configfs_path}
	 */
	USBG_TEST_TS("test_cpy_configfs_path_simple",
		     test_cpy_configfs_path, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_simple,
	 * Check if returned simple config matches original one,
	 * usbg_get_config}
	 */
	USBG_TEST_TS("test_get_config_simple",
		     test_get_config, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_without_label_simple,
	 * Check if returned simple config matches original one,
	 * usbg_get_config}
	 */
	USBG_TEST_TS("test_get_config_without_label_simple",
		     test_get_config_without_label, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_fail,
	 * Check if trying to get non-existing or invalid config
	 * fails as expected,
	 * usbg_get_config}*/
	USBG_TEST_TS("test_get_config_fail",
		     test_get_config_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_label_simple,
	 * Check if returned simple config label matches original one,
	 * usbg_get_config_label}
	 */
	USBG_TEST_TS("test_get_config_label_simple",
		     test_get_config_label, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_id_simple,
	 * Check if returned simple config id matches original one,
	 * usbg_get_config_id}
	 */
	USBG_TEST_TS("test_get_config_id_simple",
		     test_get_config_id, setup_simple_state),

#ifndef DOXYGEN
};
#endif

/**
 * @usbg_test
 * @tests_end
 */

#define TESTS_TAG "tests"
/* for autotools compability */
#define SKIPPED_CODE 77

#ifdef HAS_LIBCONFIG

static int gen_test_config(FILE *output)
{
	config_t cfg;
	config_setting_t *root;
	config_setting_t *tests_node, *node;
	int i;
	int ret = SKIPPED_CODE, cfg_ret = 0;

	config_init(&cfg);
	config_set_tab_width(&cfg, 4);

	root = config_root_setting(&cfg);
	tests_node = config_setting_add(root, TESTS_TAG, CONFIG_TYPE_LIST);
	if (!tests_node) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

		node = config_setting_add(tests_node, NULL, CONFIG_TYPE_STRING);
		if (!node) {
			ret = -ENOMEM;
			goto out;
		}

		cfg_ret = config_setting_set_string(node, tests[i].name);
		if (cfg_ret != CONFIG_TRUE) {
			ret = -EINVAL;
			goto out;
		}
	}

	config_write(&cfg, output);
out:
	config_destroy(&cfg);
	return ret;
}

#else

static int gen_test_config(FILE *output)
{
	fprintf(stderr, "Libconfig is not supported\n");
	return -ENOTSUP;
}

#endif /* HAS_LIBCONFIG */

static int lookup_test(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(tests); ++i)
		if (tests[i].function_type == UNIT_TEST_FUNCTION_TYPE_TEST &&
		    !strcmp(name, tests[i].name))
			return i;

	return -1;
}

#ifdef HAS_LIBCONFIG
static int apply_test_config(FILE *input)
{
	config_t cfg;
	config_setting_t *root;
	config_setting_t *tests_node, *node;
	int i, count, ind;
	int ret = 0, cfg_ret = 0;
	const char *test_name;
	char selected[ARRAY_SIZE(tests)];

	for (i = 0; i < ARRAY_SIZE(selected); ++i)
		selected[i] = 0;

	config_init(&cfg);

	cfg_ret = config_read(&cfg, input);
	if (cfg_ret != CONFIG_TRUE) {
		fprintf(stderr, "Wrong config format\n");
		ret = -EINVAL;
		goto out;
	}

	root = config_root_setting(&cfg);
	tests_node = config_setting_get_member(root, TESTS_TAG);
	if (!tests_node || !config_setting_is_list(tests_node)) {
		fprintf(stderr, "Missing or incorrect tests list\n");
		ret = -EINVAL;
		goto out;
	}

	count = config_setting_length(tests_node);
	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(tests_node, i);
		if (!node) {
			ret = -EINVAL;
			goto out;
		}

		test_name = config_setting_get_string(node);
		if (!test_name) {
			fprintf(stderr, "Incorrect tests list. Element %d\n", i);
			ret = -EINVAL;
			goto out;
		}

		ind = lookup_test(test_name);
		if (ind < 0) {
			fprintf(stderr, "Test %s not found.\n", test_name);
			ret = -EINVAL;
			goto out;
		}

		selected[ind] = 1;
	}

	/* Structures with NULL function are skipped by cmocka*/
	for (i = 0; i < ARRAY_SIZE(selected); ++i) {
		if (selected[i] ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

		if (i - 1 >= 0 && tests[i - 1].function_type ==
		    UNIT_TEST_FUNCTION_TYPE_SETUP)
			tests[i - 1].function = NULL;

		tests[i].function = NULL;

		if (i + 1 < ARRAY_SIZE(tests) && tests[i + 1].function_type ==
		    UNIT_TEST_FUNCTION_TYPE_TEARDOWN)
			tests[i + 1].function = NULL;
	}
out:
	config_destroy(&cfg);
	return ret;
}

#else

static int apply_test_config(FILE *input)
{
	fprintf(stderr, "Libconfig is not supported\n");
	return -ENOTSUP;
}

#endif /* HAS_LIBCONFIG */

static void print_skipped_tests(FILE *stream)
{
	int i = 0, nmb_skipped = 0;

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;
		++nmb_skipped;
	}

	if (nmb_skipped == 0)
		return;

	fprintf(stream, "[==========] %d test(s) skipped.\n",
		nmb_skipped);

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

		fprintf(stream, "[ SKIPPED  ] %s\n", tests[i].name);
	}
}

static void print_help()
{
	fprintf(stderr,
		"libusbg test suit:\n"
		"    --generate-config - generates config to stdout and exit\n"
		"    --use-config - runs test suit using config from stdin\n"
		"    -h --help - print this message\n"
		);
}

int main(int argc, char **argv)
{
	enum {
		GENERATE_CONFIG = 0x01,
		USE_CONFIG = 0x02,
	};

	int options = 0;
	int opt;
	int ret = -EINVAL;

	static struct option long_options[] = {
		{"generate-config", no_argument, 0, 1},
		{"use-config", no_argument, 0, 2},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};

	while (1) {
		opt = getopt_long(argc, argv, "h", long_options, NULL);
		if (opt < 0)
			break;

		switch (opt) {
		case 1:
			options |= GENERATE_CONFIG;
			break;
		case 2:
			options |= USE_CONFIG;
			break;
		case 'h':
		default:
			print_help();
			goto out;
		}
	}

	if (optind < argc ||
	    ((options & GENERATE_CONFIG) &&
	     (options & USE_CONFIG))) {
		print_help();
		goto out;
	}

	if (options & GENERATE_CONFIG) {
		ret = gen_test_config(stdout);
		goto out;
	}

	if (options & USE_CONFIG) {
		ret = apply_test_config(stdin);
		if (ret)
			goto out;
	}

	ret = run_tests(tests);
	print_skipped_tests(stderr);

out:
	return ret;
}
