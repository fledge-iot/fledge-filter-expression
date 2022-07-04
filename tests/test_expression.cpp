#include <gtest/gtest.h>
#include <plugin_api.h>
#include <config_category.h>
#include <filter_plugin.h>
#include <filter.h>
#include <string.h>
#include <string>
#include <rapidjson/document.h>
#include <reading.h>
#include <reading_set.h>

using namespace std;
using namespace rapidjson;

extern "C" {
	PLUGIN_INFORMATION *plugin_info();
	void plugin_ingest(void *handle,
                   READINGSET *readingSet);
	PLUGIN_HANDLE plugin_init(ConfigCategory* config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output);
	int called = 0;

	void Handler(void *handle, READINGSET *readings)
	{
		called++;
		*(READINGSET **)handle = readings;
	}
};

TEST(EXPRESSION, Addition)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("expression", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("expression"), true);
	config->setValue("expression", "a + b");
	config->setValue("name", "result");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	vector<Datapoint *> datapoints;
	long a = 1000;
	DatapointValue dpv(a);
	datapoints.push_back(new Datapoint("a", dpv));
	long b = 50;
	DatapointValue dpv1(b);
	datapoints.push_back(new Datapoint("b", dpv1));
	readings->push_back(new Reading("test", datapoints));


	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);


	vector<Reading *>results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];
	ASSERT_STREQ(out->getAssetName().c_str(), "test");
	ASSERT_EQ(out->getDatapointCount(), 3);
	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 3);
	for (int i = 0; i < 3; i++)
	{
		Datapoint *outdp = points[i];
		if (outdp->getName().compare("a") == 0)
		{
			ASSERT_EQ(outdp->getData().getType(), DatapointValue::T_INTEGER);
			ASSERT_EQ(outdp->getData().toInt(), 1000);
		}
		else if (outdp->getName().compare("b") == 0)
		{
			ASSERT_EQ(outdp->getData().getType(), DatapointValue::T_INTEGER);
			ASSERT_EQ(outdp->getData().toInt(), 50);
		}
		else if (outdp->getName().compare("result") == 0)
		{
			ASSERT_EQ(outdp->getData().getType(), DatapointValue::T_FLOAT);
			ASSERT_EQ(outdp->getData().toDouble(), 1050);
		}
		else
		{
			ASSERT_STREQ(outdp->getName().c_str(), "result");
		}
	}
}
