/*
 * FogLAMP "expression" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch           
 */     
#include <reading.h>              
#include <reading_set.h>
#include <utility>                
#include <logger.h>
#include <exprtk.hpp>
#include <expression_filter.h>
#include <math.h>

using namespace std;
using namespace rapidjson;

#define MAX_VARS	20	// Maximum number of variables supported in an expression

/**
 * Construct a ExpressionFilter, call the base class constructor and handle the
 * parsing of the configuration category the required expression
 */
ExpressionFilter::ExpressionFilter(const std::string& filterName,
			       ConfigCategory& filterConfig,
                               OUTPUT_HANDLE *outHandle,
                               OUTPUT_STREAM out) :
                                  FogLampFilter(filterName, filterConfig,
                                                outHandle, out)
{
	handleConfig(filterConfig);
}

/**
 * Destructor for the filter.
 */
ExpressionFilter::~ExpressionFilter()
{
}

/**
 * Called with a set of readings, iterates over the readings applying
 * the expression to create the new data point
 *
 * @param readings	The readings to process
 */
void ExpressionFilter::ingest(const vector<Reading *>& readings)
{
exprtk::expression<double>	expression;
exprtk::symbol_table<double>	symbolTable;
exprtk::parser<double>		parser;
double				variables[MAX_VARS];
string				variableNames[MAX_VARS];
Reading				*reading;
int				varCount = 0;

	lock_guard<mutex> guard(m_configMutex);
	/* Use the first reading to work out what the variables are */
	reading = readings[0];
	vector<Datapoint *>	datapoints = reading->getReadingData();
	for (auto it = datapoints.begin(); it != datapoints.end(); it++)
	{
		DatapointValue& dpvalue = (*it)->getData();
		if (dpvalue.getType() == DatapointValue::T_INTEGER ||
				dpvalue.getType() == DatapointValue::T_FLOAT)
		{
			variableNames[varCount++] = (*it)->getName();
		}
		if (varCount == MAX_VARS)
		{
			Logger::getLogger()->error("Too many datapoints in reading");
			break;
		}
	}

	for (int i = 0; i < varCount; i++)
	{
		symbolTable.add_variable(variableNames[i], variables[i]);
	}
	symbolTable.add_constants();
	expression.register_symbol_table(symbolTable);
	parser.compile(m_expression.c_str(), expression);

	// Iterate over the readings
	for (vector<Reading *>::const_iterator reading = readings.begin();
						      reading != readings.end();
						      ++reading)
	{
		datapoints = (*reading)->getReadingData();
		for (auto it = datapoints.begin(); it != datapoints.end(); it++)
		{
			string name = (*it)->getName();
			double value = 0.0;
			DatapointValue& dpvalue = (*it)->getData();
			if (dpvalue.getType() == DatapointValue::T_INTEGER)
			{
				value = dpvalue.toInt();
			}
			else if (dpvalue.getType() == DatapointValue::T_FLOAT)
			{
				value = dpvalue.toDouble();
			}
			else
			{
				continue;	// Unsupported type
			}
			
			bool found = false;
			for (int i = 0; i < varCount; i++)
			{
				if (variableNames[i].compare(name) == 0)
				{
					variables[i] = value;
					found = true;
					break;
				}
			}
			if (found == false && varCount < MAX_VARS - 1)
			{
				// Not previously seen this data point, add it.
				variableNames[varCount] = name;
				variables[varCount] = value;
				symbolTable.add_variable(variableNames[varCount], variables[varCount]);
				varCount++;

				// We have added a new variable so must re-parse the expression
				expression.register_symbol_table(symbolTable);
				parser.compile(m_expression.c_str(), expression);
			}
		}
		try {
			double newValue = expression.value();
			// If we yield a valid result add it as a data point
			if (isnan(newValue) == false && isfinite(newValue))
			{
				DatapointValue v(newValue);
				(*reading)->addDatapoint(new Datapoint(m_dpname, v));
			}
		} catch (exception &e) {
			// Failed to evaluate expression, so just continue
			Logger::getLogger()->error("Exception processing expression %s", m_expression.c_str());
		}
	}
}

/**
 * Reconfigure the filter. We must hold the mutex here to stop the ingest
 * as we manipulate the m_scaleSet vector when recreating the scale sets
 *
 * @param conf		The new configuration to apply
 */
void ExpressionFilter::reconfigure(const string& conf)
{
	lock_guard<mutex> guard(m_configMutex);
	setConfig(conf);
	handleConfig(m_config);
}


/**
 * Handle the configuration of the plugin.
 *
 *
 * @param conf	The configuration category for the filter.
 */
void ExpressionFilter::handleConfig(const ConfigCategory& config)
{
	setExpression(config.getValue("expression"));
	setDatapointName(config.getValue("name"));
}

