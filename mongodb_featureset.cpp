// boost
#include <boost/shared_ptr.hpp>

// stl
#include <string>
#include <set>
#include <iostream>
#include <sstream>
#include <cmath>

// #include <bsoncxx/json.hpp>

// mapnik
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/json/feature_collection_grammar.hpp>
#include <mapnik/json/extract_bounding_box_grammar_impl.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_type.hpp>
#include <mapnik/geometry_types.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/util/variant.hpp>
// #include <mapnik/feature.hpp>
// mongo
#include <mongocxx/cursor.hpp>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/types/value.hpp>


#include "mongodb_featureset.hpp"

// #include <vector>

#define TILE_SIZE 384.0

mongodb_featureset::mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx,mapnik::box2d<double> const & box)
: rs_(rs),
ctx_(ctx),
cur(rs_->begin()),
feature_id_(0),
box(box),
currentx(0),
currenty(0) 
{
	tile_xmin = (int)box.minx();
   tile_xmax = (int)box.maxx() + 1;
   tile_ymin = (int)box.miny();
   tile_ymax = (int)box.maxy() + 1;
   tile_xwidth = (tile_xmax - tile_xmin) / TILE_SIZE;
   tile_yheight = (tile_ymax - tile_ymin) / TILE_SIZE;

	// grid = std::set<int>();
   int size_bytes = (int)(TILE_SIZE * TILE_SIZE + 7) / 8;
   pszBitArrays = new unsigned char[size_bytes];
   assert(pszBitArrays);
   memset(pszBitArrays, 0, size_bytes);
   count_bit_drawn = 0;

 tr_=new transcoder("utf8");

}


mongodb_featureset::mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx)
: rs_(rs),
ctx_(ctx),
cur(rs_->begin()),
feature_id_(0),
currentx(0),
currenty(0)
{    
      // grid = std::set<int>();
  pszBitArrays = NULL;
  count_bit_drawn = 0; 
  box = mapnik::box2d<double>(0,0,0,0);
  
  tr_=new transcoder("utf8");
}

mongodb_featureset::~mongodb_featureset() {
	if (pszBitArrays) delete [] pszBitArrays;
	
}

namespace {
	using box_type = mapnik::box2d<double>;
 using boxes_type = std::vector<std::pair<box_type, std::pair<std::size_t, std::size_t>>>;
 using base_iterator_type = char const*;
 const mapnik::transcoder mongodb_datasource_static_tr("utf8");
 const mapnik::json::feature_collection_grammar<base_iterator_type,mapnik::feature_impl> mongodb_datasource_static_fc_grammar(mongodb_datasource_static_tr);
 const mapnik::json::feature_grammar_callback<base_iterator_type,mapnik::feature_impl> mongodb_datasource_static_feature_callback_grammar(mongodb_datasource_static_tr);
 const mapnik::json::feature_grammar<base_iterator_type, mapnik::feature_impl> mongodb_datasource_static_feature_grammar(mongodb_datasource_static_tr);
 const mapnik::json::extract_bounding_box_grammar<base_iterator_type, boxes_type> mongodb_datasource_static_bbox_grammar;
}

mapnik::feature_ptr mongodb_featureset::convert_geometry(const bsoncxx::document::element &loc) {

	std::string type = loc["type"].get_utf8().value.to_string();

   std::vector<bsoncxx::array::element> coords;
   bsoncxx::array::view cd_view = bsoncxx::array::view(loc["coordinates"].get_array());

   for(auto i:cd_view){
     coords.push_back(i);
 }

 if (type == "Point")
  return convert_point(coords);
else if(type == "MultiPoint")
    return convert_multiPoint(coords);
    	// else if (type == "LineString")
    	//	convert_linestring(coords, feature);
    	// else if (type == "Polygon")
    	//	convert_polygon(coords, feature);
}

mapnik::feature_ptr mongodb_featureset::convert_point(const std::vector<bsoncxx::array::element> &coords) {

   mapnik::geometry::point<double> *point = new mapnik::geometry::point<double>(); 
    	// std::cout<<"after new point()"<<std::endl;

   if(coords[0].type() == bsoncxx::type::k_double){
       point->x=(double)coords[0].get_double().value;
   }else{
       point->x=(double)coords[0].get_int32().value;     
   }
   if(coords[1].type() == bsoncxx::type::k_double){
       point->y=(double)coords[1].get_double().value;
   }else{
       point->y=(double)coords[1].get_int32().value;
   }

   currentx = point->x;
   currenty = point->y;

   mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_, feature_id_));

   feature->set_geometry(*point);

   return feature;
}


mapnik::feature_ptr mongodb_featureset::convert_multiPoint(const std::vector<bsoncxx::array::element> &coords) {
    mapnik::geometry::multi_point<double> * multi_point = new mapnik::geometry::multi_point<double>(); 

    //coords = [[1,2],[3,4],[5,6]]
    for(auto innercoords:coords){
        int coordsFlag = 0x0;
        double xCoord,yCoord;
        for(auto cd : bsoncxx::array::view(innercoords.get_array())){
            if(coordsFlag ^= 0x1){
                // x 
                if(cd.type() == bsoncxx::type::k_double){
                    xCoord=(double)cd.get_double().value;
                }else if(cd.type() == bsoncxx::type::k_int32){
                    xCoord=(double)cd.get_int32().value;     
                }

            }
            else{
                // y
                if(cd.type() == bsoncxx::type::k_double){
                    yCoord=(double)cd.get_double().value;
                }else if(cd.type() == bsoncxx::type::k_int32){
                    yCoord=(double)cd.get_int32().value;
                }
                

                multi_point->add_coord(xCoord,yCoord);

            }
        }
    }

    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_, feature_id_));

    feature->set_geometry(*multi_point);

    return feature;
}


bool mongodb_featureset::shouldDrawPt(double x, double y){

   int ptInGridX = fmod((x - tile_xmin), tile_xwidth) == 0 ? (int)((x - tile_xmin) / tile_xwidth) : (int)((x - tile_xmin) / tile_xwidth) + 1;
   int ptInGridY = fmod((y - tile_ymin), tile_yheight) == 0? (int)((y - tile_ymin) / tile_yheight) : (int)((y - tile_ymin) / tile_yheight) + 1;

   int coordinate = ptInGridY + ptInGridX * ((int) TILE_SIZE);

   int byte = (coordinate >> 3); 
   int bit = coordinate % 8;
   unsigned char MASK = (0x1 << bit);

   if ((pszBitArrays[byte] & MASK) == 0x00) {
	// if(grid.count(coordinate) == 0){
      pszBitArrays[byte] |= MASK;

    // grid.insert(coordinate);
      count_bit_drawn ++;
      return true;
  }else{
   return false;
}
}

bool mongodb_featureset::shouldBreak(){

	return count_bit_drawn == (int)(TILE_SIZE * TILE_SIZE); 
}
mapnik::feature_ptr mongodb_featureset::next() {
	// std::cout<<"call next"<<std::endl;
   while (cur != rs_->end()) {

     mapnik::feature_ptr feature;

     const bsoncxx::document::view & bson = *cur;
     bsoncxx::document::element geom = bson["geometry"];
     bsoncxx::document::element prop = bson["properties"];

     if (geom.type() != bsoncxx::type::k_document)
       continue;

   feature = mongodb_featureset::convert_geometry(geom);


        	// if(!shouldDrawPt(currentx, currenty)){

         //    	if(shouldBreak()){
         //        		break;
         //    	}else{
         //        		cur ++;
         //        		++feature_id_;

         //        		continue;
         //    		}
        	// }
            // std::cout<<"should draw"<<std::endl;


		//parse attribute
		
            if (prop.type() == bsoncxx::type::k_document)

                for (bsoncxx::document::view::const_iterator i = prop.get_document().view().begin(); i!=prop.get_document().view().end();i++) {
                    bsoncxx::document::element e = *i;

                    std::string name = e.key();
                    
                    switch (e.type()) {
                    
                    case bsoncxx::type::k_utf8:
                        feature->put_new<mapnik::value_unicode_string>(name, e.get_utf8().value.to_string());
                        break;
                    case bsoncxx::type::k_double:
                        feature->put_new(name, e.get_double().value);
                        break;
                    case bsoncxx::type::k_int64:
                        feature->put_new(name, e.get_int64().value);
                        break;

                    case bsoncxx::type::k_int32:
                        feature->put_new(name, e.get_int32().value);
                        break;
                    default:
                        // std::cout << "ignored" << std::endl;
                        break;
                    }
                }
                
   cur++;
   ++feature_id_;
   return feature;
}

return mapnik::feature_ptr();
}
