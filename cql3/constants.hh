/*
 * Copyright (C) 2015-present ScyllaDB
 *
 * Modified by ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#pragma once

#include "cql3/abstract_marker.hh"
#include "cql3/query_options.hh"
#include "cql3/update_parameters.hh"
#include "cql3/operation.hh"
#include "cql3/values.hh"
#include "mutation.hh"
#include <seastar/core/shared_ptr.hh>

namespace service::broadcast_tables {
    class update_query;
}

namespace cql3 {

/**
 * Static helper methods and classes for constants.
 */
class constants {
public:
#if 0
    private static final Logger logger = LoggerFactory.getLogger(Constants.class);
#endif
public:
    class setter : public operation {
    public:
        using operation::operation;

        virtual void execute(mutation& m, const clustering_key_prefix& prefix, const update_parameters& params) override {
            auto value = expr::evaluate(*_e, params._options);
            execute(m, prefix, params, column, value.view());
        }

        static void execute(mutation& m, const clustering_key_prefix& prefix, const update_parameters& params, const column_definition& column, cql3::raw_value_view value) {
            if (value.is_null()) {
                m.set_cell(prefix, column, params.make_dead_cell());
            } else if (value.is_value()) {
                m.set_cell(prefix, column, params.make_cell(*column.type, value));
            }
        }

        virtual void prepare_for_broadcast_tables(service::broadcast_tables::update_query& query) const override;
    };

    struct adder final : operation {
        using operation::operation;

        virtual void execute(mutation& m, const clustering_key_prefix& prefix, const update_parameters& params) override {
            auto value = expr::evaluate(*_e, params._options);
            if (value.is_null()) {
                throw exceptions::invalid_request_exception("Invalid null value for counter increment");
            } else if (value.is_unset_value()) {
                return;
            }
            auto increment = value.view().deserialize<int64_t>(*long_type);
            m.set_cell(prefix, column, params.make_counter_update_cell(increment));
        }
    };

    struct subtracter final : operation {
        using operation::operation;

        virtual void execute(mutation& m, const clustering_key_prefix& prefix, const update_parameters& params) override {
            auto value = expr::evaluate(*_e, params._options);
            if (value.is_null()) {
                throw exceptions::invalid_request_exception("Invalid null value for counter increment");
            } else if (value.is_unset_value()) {
                return;
            }
            auto increment = value.view().deserialize<int64_t>(*long_type);
            if (increment == std::numeric_limits<int64_t>::min()) {
                throw exceptions::invalid_request_exception(format("The negation of {:d} overflows supported counter precision (signed 8 bytes integer)", increment));
            }
            m.set_cell(prefix, column, params.make_counter_update_cell(-increment));
        }
    };

    class deleter : public operation {
    public:
        deleter(const column_definition& column)
            : operation(column, std::nullopt)
        { }

        virtual void execute(mutation& m, const clustering_key_prefix& prefix, const update_parameters& params) override;
    };
};

}
