
#ifndef MONGODB_FEATURESET_HPP
#define MONGODB_FEATURESET_HPP

#include <mapnik/feature.hpp>
#include "mongodb_datasource.hpp"

class mongodb_featureset : public mapnik::Featureset 
{
public:
    mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx);
    ~mongodb_featureset();

    mapnik::feature_ptr next();

	 mapnik::feature_ptr convert_geometry(const bsoncxx::document::element &loc);
	 mapnik::feature_ptr convert_point(const std::vector<bsoncxx::array::element> &) ;
private:
    std::shared_ptr<mongocxx::cursor> rs_;
    mongocxx::cursor::iterator cur;
    mapnik::context_ptr  ctx_;
    mapnik::value_integer feature_id_ = 1;
};

#endif // MONGODB_FEATURESET_HPP
