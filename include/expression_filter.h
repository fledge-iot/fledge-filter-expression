#ifndef _EXPRESSION_FILTER_H
#define _EXPRESSION_FILTER_H
/*
 * FogLAMP "expression" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch           
 */     
#include <filter.h>               
#include <reading_set.h>
#include <config_category.h>
#include <string>                 
#include <logger.h>
#include <mutex>                 

/**
 * A FogLAMP filter that applies an expression across a set
 * of data points in a reading and creates a new data point
 * with the result of that expression.
 */
class ExpressionFilter : public FogLampFilter {
	public:
		ExpressionFilter(const std::string& filterName,
                        ConfigCategory& filterConfig,
                        OUTPUT_HANDLE *outHandle,
                        OUTPUT_STREAM out);
		~ExpressionFilter();
		void	setExpression(const std::string& expression)
			{
				m_expression = expression;
				m_report = true;
			};
		void	setDatapointName(const std::string& dpname)
			{
				m_dpname = dpname;
			};
		void	ingest(const std::vector<Reading *>& readings);
		void	reconfigure(const std::string& newConfig);
	private:
		void 		handleConfig(const ConfigCategory& conf);
		std::string	m_expression;
		std::string	m_dpname;
		std::mutex	m_configMutex;
		bool		m_report;
};


#endif
