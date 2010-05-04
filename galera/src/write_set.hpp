//
// Copyright (C) 2010 Codership Oy <info@codership.com>
//


#ifndef GALERA_WRITE_SET_HPP
#define GALERA_WRITE_SET_HPP

#include "wsdb_api.h"
#include "wsrep_api.h"
#include "gu_buffer.hpp"

#include <vector>
#include <deque>

#include <boost/unordered_map.hpp>

namespace galera
{
    class Query
    {
    public:
        Query(const void* query = 0, 
              size_t query_len = 0, 
              time_t tstamp = -1,
              uint32_t rnd_seed = 0) :
            query_(reinterpret_cast<const gu::byte_t*>(query), 
                   reinterpret_cast<const gu::byte_t*>(query) + query_len),
            tstamp_(tstamp),
            rnd_seed_(rnd_seed)
        { }
        
        const gu::Buffer& get_query() const { return query_; }
        time_t get_tstamp() const { return tstamp_; }
        uint32_t get_rnd_seed() const { return rnd_seed_; }
    private:
        friend size_t serialize(const Query&, gu::byte_t*, size_t, size_t);
        friend size_t unserialize(const gu::byte_t*, size_t, size_t, Query&);
        friend size_t serial_size(const Query&);
        gu::Buffer query_;
        time_t tstamp_;
        uint32_t rnd_seed_;
    };
    
    std::ostream& operator<<(std::ostream&, const Query& q);
    
    typedef std::vector<Query> QuerySequence;
    
    class RowKey
    {
    public:
        RowKey(const void* dbtable = 0, uint16_t dbtable_len = 0,
               const void* key = 0, uint16_t key_len = 0, gu::byte_t action = 0)
            :
            dbtable_(dbtable),
            dbtable_len_(dbtable_len),
            key_(key),
            key_len_(key_len),
            action_(action) 
        { }
        const void* get_dbtable() const { return dbtable_; }
        size_t get_dbtable_len() const { return dbtable_len_; }
        const void* get_key() const { return key_; }
        size_t get_key_len() const { return key_len_; }
    private:
        friend size_t serialize(const RowKey&, gu::byte_t*, size_t, size_t);
        friend size_t unserialize(const gu::byte_t*, size_t, size_t, RowKey&);
        friend size_t serial_size(const RowKey&);
        friend bool operator==(const RowKey& a, const RowKey& b);
        const void* dbtable_;
        uint16_t dbtable_len_;
        const void* key_;
        uint16_t key_len_;
        gu::byte_t action_;
    };
    
    inline bool operator==(const RowKey& a, const RowKey&b)
    {
        if (a.dbtable_len_ != b.dbtable_len_) return false;
        if (a.key_len_ != b.key_len_) return false;
        if (memcmp(a.dbtable_, b.dbtable_, a.dbtable_len_) != 0) return false;
        return (memcmp(a.key_, b.key_, a.key_len_) == 0);
    }
    
    typedef std::deque<RowKey> RowKeySequence;
    
        class RowKeyHash
        {
        public:
            size_t operator()(const RowKey& rk) const
            {
                const gu::byte_t* b(reinterpret_cast<const gu::byte_t*>(
                                        rk.get_key()));
                const gu::byte_t* e(reinterpret_cast<const gu::byte_t*>(
                                        rk.get_key()) + rk.get_key_len());
                return boost::hash_range(b, e);
            }
        };

    
    class WriteSet 
    {
    public:
        WriteSet() 
            : 
            type_(),
            level_(WSDB_WS_QUERY),
            last_seen_trx_(),
            queries_(),
            keys_(),
            key_refs_(),
            rbr_()
        { }
        
        WriteSet(enum wsdb_ws_type type) 
            : 
            type_(type),
            level_(WSDB_WS_QUERY),
            last_seen_trx_(),
            queries_(),
            keys_(),
            key_refs_(),
            rbr_()
        { }
        
        enum wsdb_ws_type get_type() const { return type_; }
        enum wsdb_ws_level get_level() const { return level_; }
        wsrep_seqno_t get_last_seen_trx() const { return last_seen_trx_; }
        const gu::Buffer& get_rbr() const { return rbr_; }
        
        void append_query(const void* query, size_t query_len)
        {
            queries_.push_back(Query(query,
                                     query_len));
        }
        
        void append_row_key(const void* dbtable, size_t dbtable_len,
                            const void* key, size_t key_len,
                            int action);
        
        
        void assign_rbr(const void* rbr_data, size_t rbr_data_len)
        {
            assert(rbr_.empty() == true);
            rbr_.reserve(rbr_data_len);
            rbr_.insert(rbr_.begin(),
                        reinterpret_cast<const gu::byte_t*>(rbr_data),
                        reinterpret_cast<const gu::byte_t*>(rbr_data) + rbr_data_len);
            level_ = WSDB_WS_DATA_RBR;
        }
        
        void get_keys(RowKeySequence&) const;
        const QuerySequence& get_queries() const { return queries_; }
        bool empty() const { return (rbr_.size() == 0); }
        void serialize(gu::Buffer& buf) const;
        void clear() { keys_.clear(), key_refs_.clear(),
                rbr_.clear(), queries_.clear(); }
    private:
        friend size_t serialize(const WriteSet&, gu::byte_t*, size_t, size_t);
        friend size_t unserialize(const gu::byte_t*, size_t, size_t, WriteSet&);
        friend size_t serial_size(const WriteSet&);
        
        enum wsdb_ws_type type_;
        enum wsdb_ws_level level_;
        wsrep_seqno_t last_seen_trx_;
        QuerySequence queries_;
        gu::Buffer keys_;
        typedef boost::unordered_multimap<size_t, size_t> KeyRefMap;
        KeyRefMap key_refs_;
        gu::Buffer rbr_;
    };
}


#endif // GALERA_WRITE_SET_HPP