
#ifndef MONGODB_FEATURESET_HPP
#define MONGODB_FEATURESET_HPP

#include <mapnik/feature.hpp>
#include "mongodb_datasource.hpp"
#include <set>

class mongodb_featureset : public mapnik::Featureset 
{
public:
	mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx,mapnik::box2d<double> const &);
	mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx);
	~mongodb_featureset();

 	mapnik::feature_ptr next();

	mapnik::feature_ptr convert_geometry(const bsoncxx::document::element &loc);
	mapnik::feature_ptr convert_point(const std::vector<bsoncxx::array::element> &) ;
	mapnik::feature_ptr convert_multiPoint(const std::vector<bsoncxx::array::element> &) ;
private:
    	std::shared_ptr<mongocxx::cursor> rs_;
    	mongocxx::cursor::iterator cur;
    	mapnik::context_ptr  ctx_;
    	mapnik::value_integer feature_id_ = 1;
    	mapnik::box2d<double>  box;

    	// std::set<std::string> grid;
	// std::set<int> grid;
	
	double currentx,currenty;
    	bool shouldBreak();
    	bool shouldDrawPt(double x,double y);

	unsigned char *pszBitArrays;
	int count_bit_drawn; 
    	
	int tile_xmin;
    	int tile_xmax;
    	int tile_ymin;
    	int tile_ymax;
    	double tile_xwidth;
    	double tile_yheight;
    
};

#endif // MONGODB_FEATURESET_HPP
