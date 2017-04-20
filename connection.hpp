#ifndef MONGODB_CONNECTION_HPP
#define MONGODB_CONNECTION_HPP

// mongo
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/cursor.hpp>
#include <mongocxx/exception/query_exception.hpp>
// boost
#include <boost/shared_ptr.hpp>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/options/find.hpp>

class Connection {
private:
    bool available;
    mongocxx::client mongo_client;
    std::string db_;
    std::string collection_;
public:
    Connection(const std::string &connection_str, const std::string &db, const std::string &collection)
        : db_(db), collection_(collection), available(true), mongo_client(mongocxx::uri("mongodb://" + connection_str)) {
    }

    ~Connection() {
        close();
    }

    std::shared_ptr<mongocxx::cursor> query(const std::string &query, int limit = 0, int skip = 0) {
        try {

          	using bsoncxx::builder::stream::document;
           	
           	using bsoncxx::builder::stream::finalize;
		
		auto coll =  mongo_client[db_][collection_];
		
		mongocxx::options::find opts{};
		opts.batch_size(100000);
		opts.skip(skip);
		opts.limit(limit);
		opts.max_time(std::chrono::milliseconds(120000));
        opts.no_cursor_timeout(false); 
        //opts.max_scan(1000000);   

		// opts.projection(document{} << "properties" << 0 << finalize);
		return std::make_shared<mongocxx::cursor>(coll.find(bsoncxx::from_json(query), opts));		
		
        } catch(mongocxx::query_exception  &de) {
            std::cout<<" error here"<<std::endl;
            std::string err_msg = "Fail to query mongodb: " + /* bsoncxx::to_json(json) */ query + "\n";
            throw mapnik::datasource_exception(err_msg);
        }
    }

    bool isOK() const {
        return available;
    }

    void close() {
        available = false;
    }
};

#endif // MONGODB_CONNECTION_HPP
