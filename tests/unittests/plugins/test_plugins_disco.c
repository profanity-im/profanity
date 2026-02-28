#include "prof_cmocka.h"
#include <stdlib.h>

#include "plugins/disco.h"

void
disco_get_features__returns__empty_list_when_none(void** state)
{
    disco_close();
    GList* features = disco_get_features();

    assert_int_equal(g_list_length(features), 0);

    g_list_free(features);
    disco_close();
}

void
disco_add_feature__updates__added_feature(void** state)
{
    disco_close();
    disco_add_feature("my_plugin", "some:feature:example");

    GList* features = disco_get_features();
    assert_int_equal(g_list_length(features), 1);
    char* feature = features->data;
    assert_string_equal(feature, "some:feature:example");

    g_list_free(features);
    disco_close();
}

void
disco_close__updates__resets_features(void** state)
{
    disco_close();
    disco_add_feature("my_plugin", "some:feature:example");

    GList* features = disco_get_features();
    assert_int_equal(g_list_length(features), 1);
    g_list_free(features);

    disco_close();
    features = disco_get_features();
    assert_int_equal(g_list_length(features), 0);

    g_list_free(features);
    disco_close();
}

void
disco_get_features__returns__all_added_features(void** state)
{
    disco_close();
    disco_add_feature("first_plugin", "first:feature");
    disco_add_feature("first_plugin", "second:feature");
    disco_add_feature("second_plugin", "third:feature");
    disco_add_feature("third_plugin", "fourth:feature");
    disco_add_feature("third_plugin", "fifth:feature");

    GList* features = disco_get_features();

    assert_int_equal(g_list_length(features), 5);
    assert_true(g_list_find_custom(features, "first:feature", (GCompareFunc)g_strcmp0));
    assert_true(g_list_find_custom(features, "second:feature", (GCompareFunc)g_strcmp0));
    assert_true(g_list_find_custom(features, "third:feature", (GCompareFunc)g_strcmp0));
    assert_true(g_list_find_custom(features, "fourth:feature", (GCompareFunc)g_strcmp0));
    assert_true(g_list_find_custom(features, "fifth:feature", (GCompareFunc)g_strcmp0));

    g_list_free(features);
    disco_close();
}

void
disco_add_feature__updates__not_duplicate_feature(void** state)
{
    disco_close();
    disco_add_feature("my_plugin", "my:feature");
    disco_add_feature("some_other_plugin", "my:feature");

    GList* features = disco_get_features();
    assert_int_equal(g_list_length(features), 1);

    g_list_free(features);
    disco_close();
}

void
disco_remove_features__updates__removes_plugin_features(void** state)
{
    disco_close();
    disco_add_feature("plugin1", "plugin1:feature1");
    disco_add_feature("plugin1", "plugin1:feature2");
    disco_add_feature("plugin2", "plugin2:feature1");

    GList* features = disco_get_features();
    assert_int_equal(g_list_length(features), 3);
    g_list_free(features);

    disco_remove_features("plugin1");

    features = disco_get_features();
    assert_int_equal(g_list_length(features), 1);

    g_list_free(features);
    disco_close();
}

void
disco_remove_features__updates__not_remove_when_more_than_one_reference(void** state)
{
    disco_close();
    disco_add_feature("plugin1", "feature1");
    disco_add_feature("plugin1", "feature2");
    disco_add_feature("plugin2", "feature1");

    GList* features = disco_get_features();
    assert_int_equal(g_list_length(features), 2);
    g_list_free(features);

    disco_remove_features("plugin1");

    features = disco_get_features();
    assert_int_equal(g_list_length(features), 1);

    g_list_free(features);
    disco_close();
}
