
#include "mongodb_datasource.hpp"
#include "mongodb_featureset.hpp"
#include "connection_manager.hpp"

// mapnik
#include <mapnik/boolean.hpp>
#include <mapnik/feature.hpp>

// stl
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <boost/make_shared.hpp>

 
using mapnik::datasource;
using mapnik::parameters;

DATASOURCE_PLUGIN(mongodb_datasource)

using mapnik::attribute_descriptor;

mongodb_datasource::mongodb_datasource(parameters const& params)
    : datasource(params),
      desc_(*params.get<std::string>("type"), "utf-8"),
      type_(datasource::Vector),
      
      queryOptions_(*params.get<std::string>("queryOptions", "{}")),
      
      creator_(params.get<std::string>("host", "127.0.0.1"),
               params.get<std::string>("port", "27017"),
               params.get<std::string>("dbname", "gis"), 
               params.get<std::string>("collection"), 
               params.get<std::string>("user"),
               params.get<std::string>("password")),

      persist_connection_(*params.get<mapnik::boolean_type>("persist_connection", true)),
      extent_initialized_(false) 
      {
    	if (!params.get<std::string>("collection"))
		  throw mapnik::datasource_exception("MongoDB Plugin: missing <collection> parameter");
    	boost::optional<std::string> ext = params.get<std::string>("extent");
    	if (ext && !ext->empty())
      	extent_initialized_ = extent_.from_string(*ext);

    	boost::optional<mapnik::value_integer> initial_size = params.get<mapnik::value_integer>("initial_size", 6);
    	boost::optional<mapnik::value_integer> max_size = params.get<mapnik::value_integer>("max_size", 100);
    	
      std::cout<<"datasource creator!"<<std::endl;

      ConnectionManager::instance().registerPool(creator_, *initial_size, *max_size);

}

mongodb_datasource::~mongodb_datasource() {
    if (!persist_connection_) {
        std::shared_ptr< Pool<Connection, ConnectionCreator> > pool = ConnectionManager::instance().getPool(creator_.id());
             
        if (pool) {
            std::shared_ptr<Connection> conn = pool->borrowObject();
            if (conn)
                conn->close();
        }
    }
}

const char *mongodb_datasource::name() {
    return "mongodb";
}

mapnik::datasource::datasource_t mongodb_datasource::type() const {
    return type_;
}

mapnik::layer_descriptor mongodb_datasource::get_descriptor() const {
    return desc_;
}

std::string mongo_query(std::string queryOption, double minx, double miny, double maxx, double maxy)
{

  
	
	std::ostringstream b;
	if (!queryOption.empty() && queryOption != "{}" && strncmp(queryOption.c_str(), "{" , strlen("{")) == 0 && strncmp(queryOption.c_str() + queryOption.length() - 1, "}" , strlen("}")) == 0)
	{	
		b << queryOption.substr(0, queryOption.length() - 1) << ",";
	}	
	else
		b << "{";

   	b << "\"geometry\": {\"$geoIntersects\": {\"$geometry\": {\"type\":\"Polygon\", \"coordinates\":[[[";
   	b << std::setprecision(16) << minx << "," <<  miny << "],[";
    	b << std::setprecision(16) << maxx << "," <<  miny << "],[";
     	b << std::setprecision(16) << maxx << "," <<  maxy << "],[";
     	b << std::setprecision(16) << minx << "," <<  maxy << "],[";
     	b << std::setprecision(16) << minx << "," <<  miny << "]]]}}}";
     	b << "}";
	return b.str();
}

mapnik::featureset_ptr mongodb_datasource::features(const mapnik::query &q) const {
    
    mapnik::box2d<double> const& box = q.get_bbox();
    
    std::shared_ptr< Pool<Connection, ConnectionCreator> > pool = ConnectionManager::instance().getPool(creator_.id());
    
    if (pool) {
        std::shared_ptr<Connection> conn = pool->borrowObject();
        if (!conn)
            return mapnik::featureset_ptr();
        if (conn && conn->isOK()) {
           
            std::string queryOptions = mongo_query(queryOptions_, box.minx(), box.miny(), box.maxx(), box.maxy());
            std::shared_ptr<mongocxx::cursor> rs = conn->query(queryOptions);
            mapnik::context_ptr ctx =std::make_shared<mapnik::context_type>() ; 

            return std::make_shared<mongodb_featureset>(rs,ctx);
        }
        
    }

    return mapnik::make_invalid_featureset();
}

mapnik::featureset_ptr mongodb_datasource::features_at_point(const mapnik::coord2d &pt, double tol) const {
    std::shared_ptr< Pool<Connection, ConnectionCreator> > pool = ConnectionManager::instance().getPool(creator_.id());
      std::cout<<" borrowObject by teatures_at_point"<<std::endl;
    if (pool) {
        std::shared_ptr<Connection> conn = pool->borrowObject();

        if (!conn)
            return mapnik::featureset_ptr();

        if (conn->isOK()) {
           
    std::string queryOptions = mongo_query(queryOptions_, pt.x - tol, pt.y - tol, pt.x + tol, pt.y + tol);
  
    std::shared_ptr<mongocxx::cursor> rs(conn->query(queryOptions/*, fieldConfig */));
    mapnik::context_ptr ctx =std::make_shared<mapnik::context_type>() ; 
 
    return std::make_shared<mongodb_featureset>(rs,ctx);
        }
    }

    return mapnik::make_invalid_featureset();
}

mapnik::box2d<double> mongodb_datasource::envelope() const {
    if (extent_initialized_)
        return extent_;
    else{

          extent_.init(75, 10, 135, 60);
          extent_initialized_ = true;
          return extent_;
        }

}

boost::optional<mapnik::datasource_geometry_t> mongodb_datasource::get_geometry_type() const {
  boost::optional<mapnik::datasource_geometry_t> result;

            std::cout<<" borrowObject by get_geometry_type"<<std::endl;
std::shared_ptr< Pool<Connection, ConnectionCreator> > pool = ConnectionManager::instance().getPool(creator_.id());
      if (pool) {
        std::shared_ptr<Connection> conn = pool->borrowObject();

          if (!conn)
              return result;

          if (conn->isOK()) {

      std::shared_ptr<mongocxx::cursor> cur = conn->query("{geometry: {\"$exists\": true }}");
      try {
            mongocxx::cursor::iterator itr(cur->begin());
                    if (itr != cur->end()) {
                        const bsoncxx::document::view &bson = * itr;  
          std::string type = bson["geometry"]["type"].get_utf8().value.to_string();

                        if (type == "Point")
                              result.reset(mapnik::datasource_geometry_t::Point);
          else if (type == "MultiPoint")
            result.reset(mapnik::datasource_geometry_t::Point); 
                        else if (type == "LineString")
                              result.reset(mapnik::datasource_geometry_t::LineString);
          else if (type == "MultiLineString")
            result.reset(mapnik::datasource_geometry_t::LineString);
                    		else if (type == "Polygon")
                        			result.reset(mapnik::datasource_geometry_t::Polygon);
			 		else if (type == "MultiPolygon")
						result.reset(mapnik::datasource_geometry_t::Polygon);
			 		else if (type == "GeometryCollection")
						result.reset(mapnik::datasource_geometry_t::Collection);
                    		}
            	} catch(mongocxx::exception &de) {
                		std::string err_msg = "Mongodb Plugin: mongodb_datasource::get_geometry_type()\n";
                		throw mapnik::datasource_exception(err_msg);
            		}
        	}
    	}

    	return result;
}
