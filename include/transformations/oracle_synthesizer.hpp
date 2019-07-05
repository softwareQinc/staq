/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include "synthesis/logic_synthesis.hpp"

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  /* \brief! Synthesizes all declared oracles over standard library gates
   *
   * Visits an AST and synthesizes any declared oracles,
   * replacing them with regular gate declarations which may
   * optionally declare local ancillas 
   */
  void expand_oracles(ast_context*);

  /* Implementation */
  class oracle_synthesizer final : public replacer<oracle_synthesizer> {
  public:
    using replacer<oracle_synthesizer>::visit;
    
    oracle_synthesizer(ast_context* ctx) : ctx_(ctx) {}
    ~oracle_synthesizer() {}

    std::optional<ast_node_list> replace(decl_oracle* node) {
      auto decl_builder = decl_gate::builder(ctx_, node->location(), node->identifier());
      decl_builder.add_arguments(&node->arguments());

      auto l_net = read_from_file(node->target());
      auto body = synthesize(ctx_, node->location(), l_net, static_cast<list_ids*>(&node->arguments()));
      decl_builder.add_body(body);

      ast_node_list ret;
      ret.push_back(&node->parent(), decl_builder.finish());

      return ret;
    }

  private:
    ast_context* ctx_;
  };

  void expand_oracles(ast_context* ctx) {
    auto expander = oracle_synthesizer(ctx);
    expander.visit(*ctx);
  }
    
}
}
