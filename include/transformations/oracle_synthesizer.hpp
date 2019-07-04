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

  /* \brief! Synthesizes all declared oracles over standard library gates
   *
   * Visits an AST and synthesizes any declared oracles,
   * replacing them with regular gate declarations which may
   * optionally declare local ancillas 
   */
  void expand_oracles(qasm::ast_context*);

  /* Implementation */
  class oracle_synthesizer final : public qasm::replacer<oracle_synthesizer> {
  public:
    using replacer<oracle_synthesizer>::visit;
    
    oracle_synthesizer(qasm::ast_context* ctx) : ctx_(ctx) {}
    ~oracle_synthesizer() {}

    qasm::ast_node* replace(qasm::decl_oracle* node) {
      auto decl_builder = qasm::decl_gate::builder(ctx_, node->location(), node->identifier());
      decl_builder.add_arguments(&node->arguments());

      auto l_net = read_from_file(node->target());
      auto body = synthesize(ctx_, node->location(), l_net, static_cast<qasm::list_ids*>(&node->arguments()));
      decl_builder.add_body(body);

      return decl_builder.finish();
    }

  private:
    qasm::ast_context* ctx_;
  };

  void expand_oracles(qasm::ast_context* ctx) {
    auto expander = oracle_synthesizer(ctx);
    expander.visit(*ctx);
  }
    
}
}
