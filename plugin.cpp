/*
 * Fledge "expression" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Massimiliano Pinto
 */

#include <plugin_api.h>
#include <config_category.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <iostream>
#include <filter_plugin.h>
#include <expression_filter.h>
#include <version.h>

#define FILTER_NAME "expression"
#define DEFAULT_CONFIG "{\"plugin\" : { \"description\" : \"Apply an expression to the data stream\", " \
                       		"\"type\" : \"string\", " \
				"\"default\" : \"" FILTER_NAME "\", \"readonly\" : \"true\" }, " \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the scale filter.\", " \
				"\"type\": \"boolean\", " \
				"\"displayName\" : \"Enabled\", " \
				"\"default\": \"false\" }, " \
			 "\"expression\": {\"description\": \"Expression to apply\", " \
				"\"type\": \"string\", " \
				"\"default\": \"log(x)\", \"displayName\" : \"Expression to apply\", " \
				"\"order\" : \"2\" }, " \
			 "\"name\": {\"description\": \"The name of the new data point\", " \
				"\"type\": \"string\", \"displayName\" : \"Datapoint Name\", " \
				"\"order\" : \"1\", " \
				"\"default\": \"calculated\" } " \
			"}"

using namespace std;

/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
        FILTER_NAME,              // Name
        VERSION,                  // Version
        0,                        // Flags
        PLUGIN_TYPE_FILTER,       // Type
        "1.0.0",                  // Interface version
	DEFAULT_CONFIG	          // Default plugin configuration
};

typedef struct
{
	ExpressionFilter	*handle;
	std::string	configCatName;
} FILTER_INFO;

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the ScaleSet plugin. This creates the underlying
 * object and prepares the filter for operation. This will be
 * called before any data is ingested
 *
 * @param config	The configuration category for the filter
 * @param outHandle	A handle that will be passed to the output stream
 * @param output	The output stream (function pointer) to which data is passed
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory *config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output)
{
	FILTER_INFO *info = new FILTER_INFO;
	info->handle = new ExpressionFilter(FILTER_NAME,
						*config,
						outHandle,
						output);
	info->configCatName = config->getName();
	
	return (PLUGIN_HANDLE)info;
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle	The plugin handle returned from plugin_init
 * @param readingSet	The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE *handle,
		   READINGSET *readingSet)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	ExpressionFilter *filter = info->handle;
	if (!filter->isEnabled())
	{
		// Current filter is not active: just pass the readings set
		filter->m_func(filter->m_data, readingSet);
		return;
	}

	filter->ingest(((ReadingSet *)readingSet)->getAllReadings());
	const vector<Reading *>& readings = ((ReadingSet *)readingSet)->getAllReadings();
	for (vector<Reading *>::const_iterator elem = readings.begin();
						      elem != readings.end();
						      ++elem)
	{
		AssetTracker::getAssetTracker()->addAssetTrackingTuple(info->configCatName, (*elem)->getAssetName(), string("Filter"));
	}
	filter->m_func(filter->m_data, readingSet);
}

/**
 * Reconfigure the plugin
 *
 * @param handle	The plugin handle
 * @param newConfig	The new configuration
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, const string& newConfig)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	ExpressionFilter *filter = info->handle;
	filter->reconfigure(newConfig);
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	delete info->handle;
	delete info;
}

};
