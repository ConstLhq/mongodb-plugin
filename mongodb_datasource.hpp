/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *               2013 Oleksandr Novychenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#ifndef MONGODB_DATASOURCE_HPP
#define MONGODB_DATASOURCE_HPP

// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/params.hpp>
#include <mapnik/query.hpp>
#include <mapnik/feature.hpp>
// #include <mapnik/geometry/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/unicode.hpp>
// #include <mapnik/value/types.hpp>
#include <mapnik/attribute.hpp>

#pragma GCC diagnostic push
// #include <mapnik/warning_ignore.hpp>

// boost
// #include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

#include "connection_manager.hpp"

class mongodb_datasource : public mapnik::datasource {

private:
    	mapnik::layer_descriptor desc_;
    	mapnik::datasource::datasource_t type_;
    	
        ConnectionCreator<Connection> creator_;
    	

// std::shared_ptr< Pool<Connection, ConnectionCreator> > pool;

        bool persist_connection_;
    	mutable bool extent_initialized_;
    	mutable mapnik::box2d<double> extent_;
	    const std::string queryOptions_; 	
    // db.collectionName.find({}) .queryOptions should be "{}" or "{name:value, field1： value2}"
      // const std::string fieldConfig_;  	
      // db.collectionName.find(queryOptions, {}) .filedConfig_ should be "{}" or "{field1:0, field2： 1,  field3： 0}"

public:
    	mongodb_datasource(const mapnik::parameters &params);
    	~mongodb_datasource();

    	static const char *name();
    	mapnik::datasource::datasource_t type() const;
    	mapnik::layer_descriptor get_descriptor() const;

    	mapnik::featureset_ptr features(const mapnik::query &q) const;
    	mapnik::featureset_ptr features_at_point(mapnik::coord2d const &pt, double tol = 0) const;
    	mapnik::box2d<double> envelope() const;

    	boost::optional<mapnik::datasource_geometry_t> get_geometry_type() const;
};

#endif // MONGODB_DATASOURCE_HPP
