/*
 * Copyright (C) 2015-present ScyllaDB
 *
 * Modified by ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#pragma once

#include <vector>
#include "cql3/assignment_testable.hh"
#include "query-request.hh"
#include "types.hh"
#include "schema/schema_fwd.hh"
#include "counters.hh"

namespace cql3 {

namespace selection {

class result_set_builder;

/**
 * A <code>selector</code> is used to convert the data returned by the storage engine into the data requested by the
 * user. They correspond to the &lt;selector&gt; elements from the select clause.
 * <p>Since the introduction of aggregation, <code>selector</code>s cannot be called anymore by multiple threads
 * as they have an internal state.</p>
 */
class selector : public assignment_testable {
public:
    class factory;

    virtual ~selector() {}

    /**
     * Add the current value from the specified <code>result_set_builder</code>.
     *
     * @param rs the <code>result_set_builder</code>
     * @throws InvalidRequestException if a problem occurs while add the input value
     */
    virtual void add_input(result_set_builder& rs) = 0;

    /**
     * Returns the selector output.
     *
     * @return the selector output
     * @throws InvalidRequestException if a problem occurs while computing the output value
     */
    virtual bytes_opt get_output() = 0;

    /**
     * Returns the <code>selector</code> output type.
     *
     * @return the <code>selector</code> output type.
     */
    virtual data_type get_type() const = 0;

    virtual bool requires_thread() const;

    /**
     * Checks if this <code>selector</code> is creating aggregates.
     *
     * @return <code>true</code> if this <code>selector</code> is creating aggregates <code>false</code>
     * otherwise.
     */
    virtual bool is_aggregate() const {
        return false;
    }

    /**
     * Reset the internal state of this <code>selector</code>.
     */
    virtual void reset() = 0;

    virtual assignment_testable::test_result test_assignment(data_dictionary::database db, const sstring& keyspace, const column_specification& receiver) const override {
        auto t1 = receiver.type->underlying_type();
        auto t2 = get_type()->underlying_type();
        // We want columns of `counter_type' to be served by underlying type's overloads
        // (here: `counter_cell_view::total_value_type()') with an `EXACT_MATCH'.
        // Weak assignability between the two would lead to ambiguity because
        // `WEAKLY_ASSIGNABLE' counter->blob conversion exists and would compete.
        if (t1 == t2 || (t1 == counter_cell_view::total_value_type() && t2->is_counter())) {
            return assignment_testable::test_result::EXACT_MATCH;
        } else if (t1->is_value_compatible_with(*t2)) {
            return assignment_testable::test_result::WEAKLY_ASSIGNABLE;
        } else {
            return assignment_testable::test_result::NOT_ASSIGNABLE;
        }
    }
};

/**
 * A factory for <code>selector</code> instances.
 */
class selector::factory {
public:
    virtual ~factory() {}

    /**
     * Returns the column specification corresponding to the output value of the selector instances created by
     * this factory.
     *
     * @param schema the column family schema
     * @return a column specification
     */
    lw_shared_ptr<column_specification> get_column_specification(const schema& schema) const;

    /**
     * Creates a new <code>selector</code> instance.
     *
     * @return a new <code>selector</code> instance
     */
    virtual ::shared_ptr<selector> new_instance() const = 0;

    /**
     * Checks if this factory creates simple selectors instances.
     *
     * @return <code>true</code> if this factory creates simple selectors instances,
     * <code>false</code> otherwise
     */
    virtual bool is_simple_selector_factory() const {
        return false;
    }

    /**
     * Checks if arguments for this factory contains only simple slectors.
     *
     * @return <code>true</code> if this factory contains 
     * <code>false</code> otherwise, or if it isn't function selector factory
     */
    virtual bool contains_only_simple_arguments() const {
        return false;
    }

    /**
     * Checks if this factory creates selectors instances that creates aggregates.
     *
     * @return <code>true</code> if this factory creates selectors instances that creates aggregates,
     * <code>false</code> otherwise
     */
    virtual bool is_aggregate_selector_factory() const {
        return false;
    }

    virtual bool is_count_selector_factory() const {
        return false;
    }

    virtual bool is_reducible_selector_factory() const {
        return false;
    }

    virtual std::optional<std::pair<query::forward_request::reduction_type, query::forward_request::aggregation_info>> 
    get_reduction() const {return std::nullopt;}

    /**
     * Checks if this factory creates <code>writetime</code> selectors instances.
     *
     * @return <code>true</code> if this factory creates <code>writetime</code> selectors instances,
     * <code>false</code> otherwise
     */
    virtual bool is_write_time_selector_factory() const {
        return false;
    }

    /**
     * Checks if this factory creates <code>TTL</code> selectors instances.
     *
     * @return <code>true</code> if this factory creates <code>TTL</code> selectors instances,
     * <code>false</code> otherwise
     */
    virtual bool is_ttl_selector_factory() const {
        return false;
    }

    /**
     * Returns the name of the column corresponding to the output value of the selector instances created by
     * this factory.
     *
     * @return a column name
     */
    virtual sstring column_name() const = 0;

    /**
     * Returns the type of the values returned by the selector instances created by this factory.
     *
     * @return the selector output type
     */
    virtual data_type get_return_type() const = 0;
};

}

}
