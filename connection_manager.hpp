#ifndef MONGODB_CONNECTION_MANAGER_HPP
#define MONGODB_CONNECTION_MANAGER_HPP

#include "connection.hpp"

// mapnik
#include <mapnik/pool.hpp>
#include <mapnik/util/singleton.hpp>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include <mongocxx/instance.hpp>
// #include <mongocxx/stdx.hpp>
// #include <mongocxx/uri.hpp>

// stl
#include <string>
#include <iostream>
#include <unistd.h>

using mapnik::Pool;
using mapnik::singleton;
using mapnik::CreateStatic;

template <typename T>
class ConnectionCreator {
    boost::optional<std::string> host_, port_;
    boost::optional<std::string> dbname_, collection_;
    boost::optional<std::string> user_, pass_;

public:
    ConnectionCreator(const boost::optional<std::string> &host,
                      const boost::optional<std::string> &port,
                      const boost::optional<std::string> &dbname,
                      const boost::optional<std::string> &collection,
                      const boost::optional<std::string> &user,
                      const boost::optional<std::string> &pass)
        : host_(host), port_(port),
          dbname_(dbname), collection_(collection),
          user_(user), pass_(pass) {
     }

    T* operator()() const {
        std::cout << "new a connect_pool to mongodb: ["  << connection_string() << "  "  << *dbname_  << "  " <<*collection_ << std::endl;  
        return new T(connection_string(), *dbname_, *collection_);
    }

    inline std::string id() const {
        return connection_string() + " " + namespace_string();
    }

    inline std::string connection_string() const {
        std::string rs = *host_;

        if (port_ && !port_->empty())
            rs += ":" + *port_;

        return rs;
    }

    inline std::string namespace_string() const {
        return *dbname_ + "." + *collection_;
    }
};


class ConnectionManager : public singleton <ConnectionManager,CreateStatic> {
public:
    using PoolType = Pool<Connection,ConnectionCreator>;    
private:
    friend class CreateStatic<ConnectionManager>;
    using ContType = std::map<std::string, std::shared_ptr<PoolType> >;
    
    using HolderType = std::shared_ptr<Connection>;
    
    ContType pools_;

public:
    bool registerPool(const ConnectionCreator<Connection> &creator, mapnik::value_integer initialSize, mapnik::value_integer maxSize) {
        
        mapnik::datasource_exception("registerPool");
        
        std::cout<<" registerPool!!!! "<<std::endl;
        
        ContType::const_iterator itr = pools_.find(creator.id());

        if (itr != pools_.end()) {
            itr->second->set_initial_size(initialSize);
            itr->second->set_max_size(maxSize);
        } else
        {
            return pools_.insert(
                std::make_pair(creator.id(), std::make_shared<PoolType>(creator, initialSize, maxSize))).second;
        }

        return false;

    }

    std::shared_ptr<PoolType> getPool(std::string const& key) {
        
        ContType::const_iterator itr = pools_.find(key);

        if (itr != pools_.end()) 
        {

            std::cout << "SIZE "<<itr->second->size()<< std::endl;
            while(itr->second->size()>8){
    		std::cout << "SLEEP 2"<< std::endl;
                usleep(2000000);
            }
            
            return itr->second;
        }
        std::cout << "Fail to find connection" << std::endl;
        
        static const std::shared_ptr<PoolType> emptyPool;
        
        return emptyPool;
    }

    ConnectionManager() {
        mongocxx::instance instance{}; // This should be done only once.
    }

private:
    ConnectionManager(const ConnectionManager&);
    ConnectionManager &operator=(const ConnectionManager);
};

#endif // MONGODB_CONNECTION_MANAGER_HPP
