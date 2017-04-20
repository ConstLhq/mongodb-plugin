// boost
#include <boost/shared_ptr.hpp>

// stl
#include <string>

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

mongodb_featureset::mongodb_featureset(const std::shared_ptr<mongocxx::cursor> &rs,const mapnik::context_ptr &ctx)
: rs_(rs),
ctx_(ctx),
cur(rs_->begin()),
 feature_id_(0)
{    
  // std::cout<<"creat mongodb_featureset "<<std::endl;
}

mongodb_featureset::~mongodb_featureset() {
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
    bsoncxx::array::view cd_view=bsoncxx::array::view(loc["coordinates"].get_array());
   
    for(auto i:cd_view){
      coords.push_back(i);
    }

    if (type == "Point")
        return convert_point(coords);
    // else if (type == "LineString")
    //     convert_linestring(coords, feature);
    // else if (type == "Polygon")
    //     convert_polygon(coords, feature);
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

    // point->y=(double)coords[1].get_double().value;
    // std::cout<<point.x<<std::endl;
    // std::cout<<point.y<<std::endl;
    
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_, feature_id_));
    
    // std::cout<<"after feature_ptr"<<std::endl;

    feature->set_geometry(*point);

    return feature;
}

mapnik::feature_ptr mongodb_featureset::next() {
    while (cur != rs_->end()) {
        // std::cout<<"begin next"<<std::endl;
        mapnik::feature_ptr feature;
        // std::cout<<"pos 1"<<std::endl;

        // try {
           
            const bsoncxx::document::view & bson = *cur;
            bsoncxx::document::element geom = bson["geometry"];
            bsoncxx::document::element prop = bson["properties"];

            
            if (geom.type() != bsoncxx::type::k_document)
                continue;

           feature = mongodb_featureset::convert_geometry(geom);
          
//parse attribute
/*
            if (prop.type() == bsoncxx::type::k_document)

                for (bsoncxx::document::view::const_iterator i = prop.get_document().view().begin(); i!=prop.get_document().view().end();i++) {
                    bsoncxx::document::element e = *i;

                    std::string name = "test_key";// e.key();
                    switch (e.type()) {
                    
                    // case bsoncxx::type::k_utf8:
                    //     feature->put_new(name, e.get_utf8().value.to_string());
                    //     break;

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
                
                */
        // } catch(mongocxx::exception &de) {
            // std::string err_msg = "Mongodb Plugin ERROR:  ";
            // throw mapnik::datasource_exception(err_msg);
        // }

        cur++;
        ++feature_id_;
        // std::cout<<"pos 4"<<std::endl;
        return feature;
    }

    // std::cout<<"end"<<std::endl;

    return mapnik::feature_ptr();
}
