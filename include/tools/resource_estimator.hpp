/*
 * This file is part of staq.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

// TODO: Account for compound gates, i.e. qreg q[n]; reset q;

#include "ast/ast.hpp"

#include <algorithm>

namespace staq {
namespace tools {

using resource_count = std::unordered_map<std::string, int>;

void add_counts(resource_count& A, const resource_count& B) {
    for (auto& [gate, num] : B)
        A[gate] += num;
}

class ResourceEstimator final : public ast::Visitor {
  public:
    struct config {
        bool unbox = true;
        bool merge_dagger = true;
        std::set<std::string_view> overrides = ast::qelib_defs;
    };

    ResourceEstimator() = default;
    ResourceEstimator(const config& params) : Visitor(), config_(params) {}
    ~ResourceEstimator() = default;

    resource_count run(ast::ASTNode& node) {
        reset();

        node.accept(*this);

        // Unboxing the running estimate
        auto& [counts, depths] = running_estimate_;

        // Get maximum critical path length
        int depth = 0;
        for (auto& [id, length] : depths) {
            if (length > depth)
                depth = length;
        }

        // Set depth and return
        counts["depth"] = depth;
        return counts;
    }

    /* Variables */
    void visit(ast::VarAccess&) {}

    /* Expressions */
    void visit(ast::BExpr&) {}
    void visit(ast::UExpr&) {}
    void visit(ast::PiExpr&) {}
    void visit(ast::IntExpr&) {}
    void visit(ast::RealExpr&) {}
    void visit(ast::VarExpr&) {}

    /* Statements */
    void visit(ast::MeasureStmt& stmt) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        counts["measurement"] += 1;

        // Depth
        int in_depth = std::max(depths[stmt.c_arg()], depths[stmt.q_arg()]);
        depths[stmt.c_arg()] = in_depth + 1;
        depths[stmt.q_arg()] = in_depth + 1;
    }
    void visit(ast::ResetStmt& stmt) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        counts["reset"] += 1;

        // Depth
        depths[stmt.arg()] += 1;
    }
    void visit(ast::IfStmt& stmt) { stmt.then().accept(*this); }

    /* Gates */
    void visit(ast::UGate& gate) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        std::stringstream ss;
        auto theta = gate.theta().constant_eval();
        auto phi = gate.phi().constant_eval();
        auto lambda = gate.lambda().constant_eval();

        if (theta && phi && lambda)
            ss << "U(" << *theta << "," << *phi << "," << *lambda << ")";
        else
            ss << "U";

        counts[ss.str()] += 1;

        // Depth
        depths[gate.arg()] += 1;
    }
    void visit(ast::CNOTGate& gate) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        counts["CX"] += 1;

        // Depth
        int in_depth = std::max(depths[gate.ctrl()], depths[gate.tgt()]);
        depths[gate.ctrl()] = in_depth + 1;
        depths[gate.tgt()] = in_depth + 1;
    }
    void visit(ast::BarrierGate& gate) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        counts["barrier"] += 1;

        // Depth
        int in_depth = -1;
        gate.foreach_arg([&in_depth, this](auto& arg) {
            in_depth = std::max(in_depth, running_estimate_.second[arg]);
        });
        gate.foreach_arg([in_depth, this](auto& arg) {
            running_estimate_.second[arg] = in_depth + 1;
        });
    }
    void visit(ast::DeclaredGate& gate) {
        auto& [counts, depths] = running_estimate_;

        // Gate count
        auto tmp = gate.name();
        if (config_.merge_dagger)
            strip_dagger(tmp);

        std::stringstream ss;
        bool all_constant = true;
        if (gate.num_cargs() > 0) {
            bool flag = true;
            ss << "(";
            gate.foreach_carg([&ss, &flag, &all_constant](auto& arg) {
                auto val = arg.constant_eval();

                // Correct commas
                if (flag)
                    flag = false;
                else
                    ss << ",";

                if (val)
                    ss << *val;
                else
                    all_constant = false;
            });
            ss << ")";
        }

        std::string name;
        if (all_constant)
            name = tmp + ss.str();
        else
            name = tmp;

        auto& [gate_counts, depth_counts] = resource_map_[name];
        if (config_.unbox &&
            (config_.overrides.find(name) == config_.overrides.end()) &&
            (gate.num_cargs() == 0)) {
            add_counts(counts, gate_counts);

            // Note that this gives the "boxed" depth, which is not really
            // optimal In the future this should be changed
            int in_depth = -1;
            int gate_depth = gate_counts["depth"];
            gate.foreach_qarg([&in_depth, this](auto& arg) {
                in_depth = std::max(in_depth, running_estimate_.second[arg]);
            });
            gate.foreach_qarg([in_depth, this, gate_depth](auto& arg) {
                running_estimate_.second[arg] = in_depth + gate_depth;
            });
        } else {
            counts[name] += 1;
            gate.foreach_qarg(
                [this](auto& arg) { running_estimate_.second[arg] += 1; });
        }
    }

    /* Declarations */
    void visit(ast::GateDecl& decl) {
        // Initialize a new resource count
        auto& local_state = resource_map_[decl.id()];
        std::swap(running_estimate_, local_state);

        decl.foreach_stmt([this](auto& gate) { gate.accept(*this); });

        // Get maximum critical path length
        auto& [counts, depths] = running_estimate_;
        int depth = 0;
        for (auto& [id, length] : depths) {
            if (length > depth)
                depth = length;
        }

        // Set depth and return
        counts["depth"] = depth;

        std::swap(running_estimate_, local_state);
    }
    void visit(ast::OracleDecl&) {}
    void visit(ast::RegisterDecl& decl) {
        auto& [counts, depths] = running_estimate_;

        if (decl.is_quantum()) {
            counts["qubits"] += decl.size();
        } else {
            counts["cbits"] += decl.size();
        }
    }
    void visit(ast::AncillaDecl& decl) {
        auto& [counts, depths] = running_estimate_;

        if (!decl.is_dirty())
            counts["ancillas"] += decl.size();
    }

    /* Program */
    void visit(ast::Program& prog) {
        prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }

  private:
    using depth_count = std::unordered_map<ast::VarAccess, int>;
    using resource_state = std::pair<resource_count, depth_count>;

    config config_;
    std::unordered_map<std::string_view, resource_state> resource_map_;

    resource_state running_estimate_;

    void reset() {
        resource_map_.clear();
        running_estimate_.first.clear();
        running_estimate_.second.clear();
    }

    void strip_dagger(std::string& str) {
        auto len = str.size();

        if (len > 2 && str[len - 2] == 'd' && str[len - 1] == 'g') {
            str.resize(len - 2);
        }
    }
};

resource_count estimate_resources(ast::ASTNode& node) {
    ResourceEstimator estimator;
    return estimator.run(node);
}

resource_count estimate_resources(ast::ASTNode& node,
                                  const ResourceEstimator::config& params) {
    ResourceEstimator estimator(params);
    return estimator.run(node);
}

} // namespace tools
} // namespace staq
