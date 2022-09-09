/*
 * Modified by ScyllaDB
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#pragma once

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "schema_fwd.hh"
#include "utils/UUID.hh"
#include "gms/inet_address.hh"
#include "query-result-set.hh"
#include "db_clock.hh"
#include "db/commitlog/replay_position.hh"
#include "mutation_query.hh"
#include "system_keyspace_view_types.hh"
#include <map>
#include <seastar/core/distributed.hh>
#include "cdc/generation_id.hh"
#include "locator/host_id.hh"
#include "service/raft/group0_upgrade.hh"

namespace service {

class storage_proxy;
class storage_service;

namespace paxos {
    class paxos_state;
    class proposal;
} // namespace service::paxos

}

namespace netw {
    class messaging_service;
}

namespace cql3 {
    class query_processor;
    class untyped_result_set;
}

namespace gms {
    class feature;
    class feature_service;
}

namespace locator {
    class endpoint_dc_rack;
} // namespace locator

namespace gms {
    class gossiper;
}

bool is_system_keyspace(std::string_view ks_name);

namespace db {

sstring system_keyspace_name();

class config;
struct local_cache;

using system_keyspace_view_name = std::pair<sstring, sstring>;
class system_keyspace_view_build_progress;

struct truncation_record;
typedef std::vector<db::replay_position> replay_positions;

class table_selector {
public:
    static table_selector& all();
    static std::unique_ptr<table_selector> all_in_keyspace(sstring);
public:
    virtual ~table_selector() = default;
    virtual bool contains(const schema_ptr&) = 0;
    virtual bool contains_keyspace(std::string_view) = 0;
};

class system_keyspace : public seastar::peering_sharded_service<system_keyspace> {
    sharded<cql3::query_processor>& _qp;
    sharded<replica::database>& _db;
    std::unique_ptr<local_cache> _cache;

    static schema_ptr raft_config();
    static schema_ptr local();
    static schema_ptr peers();
    static schema_ptr peer_events();
    static schema_ptr range_xfers();
    static schema_ptr compactions_in_progress();
    static schema_ptr compaction_history();
    static schema_ptr sstable_activity();
    static schema_ptr large_partitions();
    static schema_ptr large_rows();
    static schema_ptr large_cells();
    static schema_ptr scylla_local();
    future<> setup_version(sharded<netw::messaging_service>& ms);
    future<> check_health();
    static future<> force_blocking_flush(sstring cfname);
    future<> build_dc_rack_info();
    future<> build_bootstrap_info();
    future<> cache_truncation_record();
    template <typename Value>
    future<> update_cached_values(gms::inet_address ep, sstring column_name, Value value);
public:
    static schema_ptr size_estimates();
public:
    static constexpr auto NAME = "system";
    static constexpr auto HINTS = "hints";
    static constexpr auto BATCHLOG = "batchlog";
    static constexpr auto PAXOS = "paxos";
    static constexpr auto BUILT_INDEXES = "IndexInfo";
    static constexpr auto LOCAL = "local";
    static constexpr auto TRUNCATED = "truncated";
    static constexpr auto PEERS = "peers";
    static constexpr auto PEER_EVENTS = "peer_events";
    static constexpr auto RANGE_XFERS = "range_xfers";
    static constexpr auto COMPACTIONS_IN_PROGRESS = "compactions_in_progress";
    static constexpr auto COMPACTION_HISTORY = "compaction_history";
    static constexpr auto SSTABLE_ACTIVITY = "sstable_activity";
    static constexpr auto SIZE_ESTIMATES = "size_estimates";
    static constexpr auto LARGE_PARTITIONS = "large_partitions";
    static constexpr auto LARGE_ROWS = "large_rows";
    static constexpr auto LARGE_CELLS = "large_cells";
    static constexpr auto SCYLLA_LOCAL = "scylla_local";
    static constexpr auto RAFT = "raft";
    static constexpr auto RAFT_SNAPSHOTS = "raft_snapshots";
    static constexpr auto RAFT_CONFIG = "raft_config";
    static constexpr auto REPAIR_HISTORY = "repair_history";
    static constexpr auto GROUP0_HISTORY = "group0_history";
    static constexpr auto DISCOVERY = "discovery";
    static constexpr auto BROADCAST_KV_STORE = "broadcast_kv_store";

    struct v3 {
        static constexpr auto BATCHES = "batches";
        static constexpr auto PAXOS = "paxos";
        static constexpr auto BUILT_INDEXES = "IndexInfo";
        static constexpr auto LOCAL = "local";
        static constexpr auto PEERS = "peers";
        static constexpr auto PEER_EVENTS = "peer_events";
        static constexpr auto RANGE_XFERS = "range_xfers";
        static constexpr auto COMPACTION_HISTORY = "compaction_history";
        static constexpr auto SSTABLE_ACTIVITY = "sstable_activity";
        static constexpr auto SIZE_ESTIMATES = "size_estimates";
        static constexpr auto AVAILABLE_RANGES = "available_ranges";
        static constexpr auto VIEWS_BUILDS_IN_PROGRESS = "views_builds_in_progress";
        static constexpr auto BUILT_VIEWS = "built_views";
        static constexpr auto SCYLLA_VIEWS_BUILDS_IN_PROGRESS = "scylla_views_builds_in_progress";
        static constexpr auto CDC_LOCAL = "cdc_local";
        static schema_ptr batches();
        static schema_ptr built_indexes();
        static schema_ptr local();
        static schema_ptr truncated();
        static schema_ptr peers();
        static schema_ptr peer_events();
        static schema_ptr range_xfers();
        static schema_ptr compaction_history();
        static schema_ptr sstable_activity();
        static schema_ptr size_estimates();
        static schema_ptr large_partitions();
        static schema_ptr scylla_local();
        static schema_ptr available_ranges();
        static schema_ptr views_builds_in_progress();
        static schema_ptr built_views();
        static schema_ptr scylla_views_builds_in_progress();
        static schema_ptr cdc_local();
    };

    struct legacy {
        static constexpr auto HINTS = "hints";
        static constexpr auto BATCHLOG = "batchlog";
        static constexpr auto KEYSPACES = "schema_keyspaces";
        static constexpr auto COLUMNFAMILIES = "schema_columnfamilies";
        static constexpr auto COLUMNS = "schema_columns";
        static constexpr auto TRIGGERS = "schema_triggers";
        static constexpr auto USERTYPES = "schema_usertypes";
        static constexpr auto FUNCTIONS = "schema_functions";
        static constexpr auto AGGREGATES = "schema_aggregates";

        static schema_ptr keyspaces();
        static schema_ptr column_families();
        static schema_ptr columns();
        static schema_ptr triggers();
        static schema_ptr usertypes();
        static schema_ptr functions();
        static schema_ptr aggregates();
        static schema_ptr hints();
        static schema_ptr batchlog();
    };

    static constexpr const char* extra_durable_tables[] = { PAXOS, SCYLLA_LOCAL, RAFT, RAFT_SNAPSHOTS, RAFT_CONFIG, DISCOVERY, BROADCAST_KV_STORE };

    static bool is_extra_durable(const sstring& name);

    // Partition estimates for a given range of tokens.
    struct range_estimates {
        schema_ptr schema;
        bytes range_start_token;
        bytes range_end_token;
        int64_t partitions_count;
        int64_t mean_partition_size;
    };

    using view_name = system_keyspace_view_name;
    using view_build_progress = system_keyspace_view_build_progress;

    static schema_ptr hints();
    static schema_ptr batchlog();
    static schema_ptr paxos();
    static schema_ptr built_indexes(); // TODO (from Cassandra): make private
    static schema_ptr raft();
    static schema_ptr raft_snapshots();
    static schema_ptr repair_history();
    static schema_ptr group0_history();
    static schema_ptr discovery();
    static schema_ptr broadcast_kv_store();

    static table_schema_version generate_schema_version(table_id table_id, uint16_t offset = 0);

    future<> setup(sharded<netw::messaging_service>& ms);
    future<> update_schema_version(table_schema_version version);

    /*
    * Save tokens used by this node in the LOCAL table.
    */
    future<> update_tokens(const std::unordered_set<dht::token>& tokens);

    /**
     * Record tokens being used by another node in the PEERS table.
     */
    future<> update_tokens(gms::inet_address ep, const std::unordered_set<dht::token>& tokens);

private:
    future<std::unordered_map<gms::inet_address, gms::inet_address>> get_preferred_ips();

public:
    template <typename Value>
    future<> update_peer_info(gms::inet_address ep, sstring column_name, Value value);

    future<> remove_endpoint(gms::inet_address ep);

    static future<> set_scylla_local_param(const sstring& key, const sstring& value);
    static future<std::optional<sstring>> get_scylla_local_param(const sstring& key);

    static std::vector<schema_ptr> all_tables(const db::config& cfg);
    static future<> make(distributed<replica::database>& db,
                         distributed<service::storage_service>& ss,
                         sharded<gms::gossiper>& g,
                         db::config& cfg,
                         table_selector& = table_selector::all());

    /// overloads

    future<foreign_ptr<lw_shared_ptr<reconcilable_result>>>
    static query_mutations(distributed<service::storage_proxy>& proxy,
                    const sstring& ks_name,
                    const sstring& cf_name);

    // Returns all data from given system table.
    // Intended to be used by code which is not performance critical.
    static future<lw_shared_ptr<query::result_set>> query(distributed<service::storage_proxy>& proxy,
                    const sstring& ks_name,
                    const sstring& cf_name);

    // Returns a slice of given system table.
    // Intended to be used by code which is not performance critical.
    static future<lw_shared_ptr<query::result_set>> query(
        distributed<service::storage_proxy>& proxy,
        const sstring& ks_name,
        const sstring& cf_name,
        const dht::decorated_key& key,
        query::clustering_range row_ranges = query::clustering_range::make_open_ended_both_sides());


    /**
     * Return a map of IP addresses containing a map of dc and rack info
     */
    std::unordered_map<gms::inet_address, locator::endpoint_dc_rack> load_dc_rack_info();
    locator::endpoint_dc_rack local_dc_rack() const;

    enum class bootstrap_state {
        NEEDS_BOOTSTRAP,
        COMPLETED,
        IN_PROGRESS,
        DECOMMISSIONED
    };

    struct compaction_history_entry {
        utils::UUID id;
        sstring ks;
        sstring cf;
        int64_t compacted_at = 0;
        int64_t bytes_in = 0;
        int64_t bytes_out = 0;
        // Key: number of rows merged
        // Value: counter
        std::unordered_map<int32_t, int64_t> rows_merged;
    };

    static future<> update_compaction_history(utils::UUID uuid, sstring ksname, sstring cfname, int64_t compacted_at, int64_t bytes_in, int64_t bytes_out,
                                       std::unordered_map<int32_t, int64_t> rows_merged);
    using compaction_history_consumer = noncopyable_function<future<>(const compaction_history_entry&)>;
    static future<> get_compaction_history(compaction_history_consumer&& f);

    struct repair_history_entry {
        utils::UUID id;
        table_id table_uuid;
        db_clock::time_point ts;
        sstring ks;
        sstring cf;
        int64_t range_start;
        int64_t range_end;
    };

    future<> update_repair_history(repair_history_entry);
    using repair_history_consumer = noncopyable_function<future<>(const repair_history_entry&)>;
    future<> get_repair_history(table_id, repair_history_consumer f);

    typedef std::vector<db::replay_position> replay_positions;

    static future<> save_truncation_record(table_id, db_clock::time_point truncated_at, db::replay_position);
    static future<> save_truncation_record(const replica::column_family&, db_clock::time_point truncated_at, db::replay_position);
    static future<replay_positions> get_truncated_position(table_id);
    static future<db::replay_position> get_truncated_position(table_id, uint32_t shard);
    static future<db_clock::time_point> get_truncated_at(table_id);
    static future<truncation_record> get_truncation_record(table_id cf_id);

    /**
     * Return a map of stored tokens to IP addresses
     *
     */
    future<std::unordered_map<gms::inet_address, std::unordered_set<dht::token>>> load_tokens();

    /**
     * Return a map of store host_ids to IP addresses
     *
     */
    future<std::unordered_map<gms::inet_address, locator::host_id>> load_host_ids();

    future<std::vector<gms::inet_address>> load_peers();

    /*
     * Read this node's tokens stored in the LOCAL table.
     * Used to initialize a restarting node.
     */
    static future<std::unordered_set<dht::token>> get_saved_tokens();

    /*
     * Gets this node's non-empty set of tokens.
     * TODO: maybe get this data from token_metadata instance?
     */
    static future<std::unordered_set<dht::token>> get_local_tokens();

    static future<std::unordered_map<gms::inet_address, sstring>> load_peer_features();

    static future<int> increment_and_get_generation();
    bool bootstrap_needed() const;
    bool bootstrap_complete() const;
    bool bootstrap_in_progress() const;
    bootstrap_state get_bootstrap_state() const;
    bool was_decommissioned() const;
    future<> set_bootstrap_state(bootstrap_state state);

    /**
     * Read the host ID from the system keyspace, creating (and storing) one if
     * none exists.
     */
    future<locator::host_id> load_local_host_id();

    /**
     * Sets the local host ID explicitly.  Should only be called outside of SystemTable when replacing a node.
     */
    future<locator::host_id> set_local_host_id(locator::host_id host_id);

    static api::timestamp_type schema_creation_timestamp();

    /**
     * Builds a mutation for SIZE_ESTIMATES_CF containing the specified estimates.
     */
    static mutation make_size_estimates_mutation(const sstring& ks, std::vector<range_estimates> estimates);

    static future<> register_view_for_building(sstring ks_name, sstring view_name, const dht::token& token);
    static future<> update_view_build_progress(sstring ks_name, sstring view_name, const dht::token& token);
    static future<> remove_view_build_progress(sstring ks_name, sstring view_name);
    static future<> remove_view_build_progress_across_all_shards(sstring ks_name, sstring view_name);
    static future<> mark_view_as_built(sstring ks_name, sstring view_name);
    static future<> remove_built_view(sstring ks_name, sstring view_name);
    static future<std::vector<view_name>> load_built_views();
    static future<std::vector<view_build_progress>> load_view_build_progress();

    // Paxos related functions
    static future<service::paxos::paxos_state> load_paxos_state(partition_key_view key, schema_ptr s, gc_clock::time_point now,
            db::timeout_clock::time_point timeout);
    static future<> save_paxos_promise(const schema& s, const partition_key& key, const utils::UUID& ballot, db::timeout_clock::time_point timeout);
    static future<> save_paxos_proposal(const schema& s, const service::paxos::proposal& proposal, db::timeout_clock::time_point timeout);
    static future<> save_paxos_decision(const schema& s, const service::paxos::proposal& decision, db::timeout_clock::time_point timeout);
    static future<> delete_paxos_decision(const schema& s, const partition_key& key, const utils::UUID& ballot, db::timeout_clock::time_point timeout);

    // CDC related functions

    /*
    * Save the CDC generation ID announced by this node in persistent storage.
    */
    static future<> update_cdc_generation_id(cdc::generation_id);

    /*
    * Read the CDC generation ID announced by this node from persistent storage.
    * Used to initialize a restarting node.
    */
    static future<std::optional<cdc::generation_id>> get_cdc_generation_id();

    static future<bool> cdc_is_rewritten();
    static future<> cdc_set_rewritten(std::optional<cdc::generation_id_v1>);

    static future<> enable_features_on_startup(sharded<gms::feature_service>& feat);

    // Load Raft Group 0 id from scylla.local
    static future<utils::UUID> get_raft_group0_id();

    // Load this server id from scylla.local
    static future<utils::UUID> get_raft_server_id();

    // Persist Raft Group 0 id. Should be a TIMEUUID.
    static future<> set_raft_group0_id(utils::UUID id);

    // Called once at fresh server startup to make sure every server
    // has a Raft ID
    static future<> set_raft_server_id(utils::UUID id);

    // Save advertised gossip feature set to system.local
    static future<> save_local_supported_features(const std::set<std::string_view>& feats);

    // Get the last (the greatest in timeuuid order) state ID in the group 0 history table.
    // Assumes that the history table exists, i.e. Raft experimental feature is enabled.
    static future<utils::UUID> get_last_group0_state_id();

    // Checks whether the group 0 history table contains the given state ID.
    // Assumes that the history table exists, i.e. Raft experimental feature is enabled.
    static future<bool> group0_history_contains(utils::UUID state_id);

    // The mutation appends the given state ID to the group 0 history table, with the given description if non-empty.
    //
    // If `gc_older_than` is provided, the mutation will also contain a tombstone that clears all entries whose
    // timestamps (contained in the state IDs) are older than `timestamp(state_id) - gc_older_than`.
    // The duration must be non-negative and smaller than `timestamp(state_id)`.
    //
    // The mutation's timestamp is extracted from the state ID.
    static mutation make_group0_history_state_id_mutation(
            utils::UUID state_id, std::optional<gc_clock::duration> gc_older_than, std::string_view description);

    // Obtain the contents of the group 0 history table in mutation form.
    // Assumes that the history table exists, i.e. Raft experimental feature is enabled.
    static future<mutation> get_group0_history(distributed<service::storage_proxy>&);

    future<service::group0_upgrade_state> load_group0_upgrade_state();
    future<> save_group0_upgrade_state(service::group0_upgrade_state);

    system_keyspace(sharded<cql3::query_processor>& qp, sharded<replica::database>& db) noexcept;
    ~system_keyspace();
    future<> start();
    future<> stop();

private:
    future<::shared_ptr<cql3::untyped_result_set>> execute_cql(const sstring& query_string, const std::initializer_list<data_value>& values);

    template <typename... Args>
    future<::shared_ptr<cql3::untyped_result_set>> execute_cql(sstring req, Args&&... args) {
        return execute_cql(req, { data_value(std::forward<Args>(args))... });
    }
}; // class system_keyspace

future<> system_keyspace_make(distributed<replica::database>& db, distributed<service::storage_service>& ss, sharded<gms::gossiper>& g, table_selector&);

} // namespace db
